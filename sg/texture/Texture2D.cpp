// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Texture2D.h"
#include <memory>
#include <sstream>
#include "rkcommon/memory/malloc.h"
#include "rkcommon/tasking/parallel_for.h"

#include "stb_image.h"
#include "tinyexr.h"
#include "tiny_dng_loader.h"

namespace ospray {
namespace sg {

// static helper functions //////////////////////////////////////////////////

//
// Generic
//
// Create the childData node given all other texture imageParams
void Texture2D::createDataNode()
{
  if (imageParams.depth == 1)
    createDataNodeType_internal<uint8_t>();
  else if (imageParams.depth == 2)
    createDataNodeType_internal<uint16_t>();
  else if (imageParams.depth == 4)
    createDataNodeType_internal<float>();
  else
    std::cerr << "#osp:sg: INVALID Texture depth " << imageParams.depth
              << std::endl;
}

template <typename T>
void Texture2D::createDataNodeType_internal()
{
  if (imageParams.components == 1)
    createDataNodeVec_internal<T>();
  else if (imageParams.components == 2)
    createDataNodeVec_internal<T, 2>();
  else if (imageParams.components == 3)
    createDataNodeVec_internal<T, 3>();
  else if (imageParams.components == 4)
    createDataNodeVec_internal<T, 4>();
  else
    std::cerr << "#osp:sg: INVALID number of texture components "
              << imageParams.components << std::endl;
}
template <typename T, int N>
void Texture2D::createDataNodeVec_internal()
{
  using vecT = vec_t<T, N>;
  // If texture doesn't use all channels(4), setup a strided-data access
  if (samplerParams.channel < 4) {
    createChildData("data",
        imageParams.size, // numItems
        sizeof(vecT) * vec2ul(1, imageParams.size.x), // byteStride
        (T *)texelData.get() + samplerParams.channel, // start address
        true);
  } else // RGBA
    createChildData(
        "data", imageParams.size, vec2ul(0, 0), (vecT *)texelData.get(), true);
}
template <typename T>
void Texture2D::createDataNodeVec_internal()
{
  createChildData(
      "data", imageParams.size, vec2ul(0, 0), (T *)texelData.get(), true);
}

OSPTextureFormat Texture2D::osprayTextureFormat(int components)
{
  if (imageParams.depth == 1) {
    if (components == 1)
      return samplerParams.preferLinear ? OSP_TEXTURE_R8 : OSP_TEXTURE_L8;
    if (components == 2)
      return samplerParams.preferLinear ? OSP_TEXTURE_RA8 : OSP_TEXTURE_LA8;
    if (components == 3)
      return samplerParams.preferLinear ? OSP_TEXTURE_RGB8 : OSP_TEXTURE_SRGB;
    if (components == 4)
      return samplerParams.preferLinear ? OSP_TEXTURE_RGBA8 : OSP_TEXTURE_SRGBA;
  } else if (imageParams.depth == 2) {
    if (components == 1)
      return imageParams.isHalf ? OSP_TEXTURE_R16F : OSP_TEXTURE_R16;
    if (components == 2)
      return imageParams.isHalf ? OSP_TEXTURE_RA16F : OSP_TEXTURE_RA16;
    if (components == 3)
      return imageParams.isHalf ? OSP_TEXTURE_RGB16F : OSP_TEXTURE_RGB16;
    if (components == 4)
      return imageParams.isHalf ? OSP_TEXTURE_RGBA16F : OSP_TEXTURE_RGBA16;
  } else if (imageParams.depth == 4) {
    if (components == 1)
      return OSP_TEXTURE_R32F;
    if (components == 2)
      return OSP_TEXTURE_RA32F;
    if (components == 3)
      return OSP_TEXTURE_RGB32F;
    if (components == 4)
      return OSP_TEXTURE_RGBA32F;
  }

  std::cerr << "#osp:sg: INVALID format " << imageParams.depth << ":"
            << components << std::endl;
  return OSP_TEXTURE_FORMAT_INVALID;
}

#ifdef USE_OPENIMAGEIO
//
// OpenImageIO
//
OIIO_NAMESPACE_USING
template <typename T>
void Texture2D::loadTexture_OIIO_readFile(std::unique_ptr<ImageInput> &in)
{
  const ImageSpec &spec = in->spec();
  const auto typeDesc = spec.format.elementtype();

  std::shared_ptr<T> data(new T[totalImageSize()],
      std::default_delete<T[]>());
  const long int stride =
      imageParams.size.x * sizeof(T) * imageParams.components;
  T *start = (T *)data.get();

  bool success =
      in->read_image(typeDesc, start, AutoStride, stride, AutoStride);
  if (success) {
    // Move shared_ptr ownership
    texelData = data;
  }
}

void Texture2D::loadTexture_OIIO(const std::string &fileName)
{
  auto in = ImageInput::open(fileName.c_str());
  if (in) {
    const ImageSpec &spec = in->spec();
    const auto typeDesc = spec.format.elementtype();

    imageParams.isHalf = (typeDesc == TypeDesc::HALF);
    imageParams.size = vec2ul(spec.width, spec.height);
    imageParams.components = spec.nchannels;
    imageParams.depth = spec.format.size();

    if (imageParams.depth == 1)
      loadTexture_OIIO_readFile<uint8_t>(in);
    else if (imageParams.depth == 2 && imageParams.isHalf)
      loadTexture_OIIO_readFile<short>(in);
    else if (imageParams.depth == 2)
      loadTexture_OIIO_readFile<uint16_t>(in);
    else if (imageParams.depth == 4)
      loadTexture_OIIO_readFile<float>(in);
    else
      std::cerr << "#osp:sg: INVALID Texture depth " << imageParams.depth
                << std::endl;

    in->close();
#if OIIO_VERSION < 10903 && OIIO_VERSION > 10603
    ImageInput::destroy(in);
#endif
  }

  if (!texelData.get()) {
    std::cerr << "#osp:sg: OpenImageIO failed to load texture '" << fileName
              << "'" << std::endl;
  }
}

#else
//
// PFM
//
void Texture2D::loadTexture_PFM_readFile(FILE *file, float scaleFactor)
{
  size_t size = totalImageSize();
  const size_t dataSize = sizeof(size) * sizeof(float);
  std::shared_ptr<float> data(new float[size], std::default_delete<float[]>());

  int rc = fread(data.get(), dataSize, 1, file);
  if (rc) {
    // Scale texels by scale factor
    float *texels = (float *)data.get();
    for (size_t i = 0; i < imageParams.size.product(); i++)
      texels[i] *= scaleFactor;

    // Move shared_ptr ownership
    texelData = data;
  }
}

void Texture2D::loadTexture_PFM(const std::string &fileName)
{
  FILE *file = nullptr;
  try {
    // Note: the PFM file specification does not support comments thus we
    // don't skip any http://netpbm.sourceforge.net/doc/pfm.html
    int rc = 0;
    file = fopen(fileName.c_str(), "rb");
    if (!file) {
      throw std::runtime_error(
          "#ospray_sg: could not open texture file '" + fileName + "'.");
    }
    // read format specifier:
    // PF: color floating point image
    // Pf: grayscale floating point image
    char format[2] = {0};
    if (fscanf(file, "%c%c\n", &format[0], &format[1]) != 2)
      throw std::runtime_error("could not fscanf");

    if (format[0] != 'P' || (format[1] != 'F' && format[1] != 'f')) {
      throw std::runtime_error(
          "#ospray_sg: invalid pfm texture file, header is not PF or "
          "Pf");
    }

    imageParams.components = 3;
    if (format[1] == 'f') {
      imageParams.components = 1;
    }

    // read width and height
    int width = -1;
    int height = -1;
    rc = fscanf(file, "%i %i\n", &width, &height);
    if (rc != 2 || width < 0 || height < 0) {
      throw std::runtime_error(
                "#ospray_sg: could not parse width and height in PF PFM file "
                "'" +
                fileName +
                "'. "
                "Please report this bug at ospray.github.io, and include named "
                "file to reproduce the error.");
    }

    // read scale factor/endianness
    float scaleEndian = 0.0;
    rc = fscanf(file, "%f\n", &scaleEndian);

    if (rc != 1) {
      throw std::runtime_error(
                "#ospray_sg: could not parse scale factor/endianness in PF "
                "PFM file '" +
                fileName +
                "'. "
                "Please report this bug at ospray.github.io, and include named "
                "file to reproduce the error.");
    }
    if (scaleEndian == 0.0) {
      throw std::runtime_error(
          "#ospray_sg: scale factor/endianness in PF PFM file can not be 0");
    }
    if (scaleEndian > 0.0) {
      throw std::runtime_error(
                "#ospray_sg: could not parse PF PFM file '" + fileName +
                "': currently supporting only little endian formats"
                "Please report this bug at ospray.github.io, and include named "
                "file to reproduce the error.");
    }

    float scaleFactor = std::abs(scaleEndian);
    imageParams.size = vec2ul(width, height);
    imageParams.depth = 4; // pfm is always float

    loadTexture_PFM_readFile(file, scaleFactor);

    if (!texelData.get())
      std::cerr << "#osp:sg: INVALID FORMAT PFM " << imageParams.components
                << std::endl;

  } catch (const std::runtime_error &e) {
    std::cerr << "#osp:sg: INVALID PFM" << std::endl;
    std::cerr << e.what() << std::endl;
  }

  if (file)
    fclose(file);

  if (!texelData.get()) {
    std::cerr << "#osp:sg: PFM failed to load texture '" << fileName << "'"
              << std::endl;
  }
}

//
// EXR (via tinyEXR)
//
template <typename T>
void Texture2D::loadTexture_EXR_interleaveImage(
    int numChannels, unsigned char **images, std::vector<int> &channelMap)
{
  std::shared_ptr<T> data(new T[totalImageSize()], std::default_delete<T[]>());
  T *texels = data.get();
  T **slices = (T **)images;
  size_t numRows = imageParams.size.y;

  // Fill texels with image[channels][pixels], interleaving channels into a
  // single image
  tasking::parallel_for(numRows, [&](size_t row) {
    T *rowTexels = texels + row * imageParams.size.x * imageParams.components;
    for (size_t x = 0; x < imageParams.size.x; x++) {
      for (size_t c = 0; c < numChannels; c++) {
        if (channelMap[c] >= 0)
          *rowTexels++ = slices[channelMap[c]][row * imageParams.size.x + x];
      }
    }
  });

  // Move shared_ptr ownership
  texelData = data;
}

void Texture2D::loadTexture_EXR(const std::string &fileName)
{
  int width;
  int height;
  const char *err = nullptr;
  const char *cFN = fileName.c_str();

  auto errTinyEXR = [](const char *err) {
    if (err) {
      fprintf(stderr, "ERR : %s\n", err);
      std::cerr << "#osp:sg: failed to load EXR texture: " << err << std::endl;
      FreeEXRErrorMessage(err); // release memory of error message.
    }
  };

  EXRVersion version;
  EXRHeader header;
  EXRImage image;
  InitEXRHeader(&header);
  InitEXRImage(&image);
  if (ParseEXRVersionFromFile(&version, cFN) != TINYEXR_SUCCESS) {
    errTinyEXR(err);
    return;
  }
  if (ParseEXRHeaderFromFile(&header, &version, cFN, &err) != TINYEXR_SUCCESS) {
    errTinyEXR(err);
    FreeEXRHeader(&header);
    return;
  }
  if (LoadEXRImageFromFile(&image, &header, cFN, &err) != TINYEXR_SUCCESS) {
    errTinyEXR(err);
    FreeEXRHeader(&header);
    FreeEXRImage(&image);
    return;
  }

  // Only handle scanline format files, don't handle tiled formats
  if (header.tiled) {
    std::cerr << "#osp:sg:EXR Unsupported tiled format: " << cFN << std::endl;
    FreeEXRHeader(&header);
    FreeEXRImage(&image);
    return;
  }

  const int MaxChannels = 4;
  imageParams.size = vec2ul(image.width, image.height);
  imageParams.components = std::min(header.num_channels, MaxChannels);
  imageParams.isHalf = (header.pixel_types[0] == TINYEXR_PIXELTYPE_HALF);
  imageParams.depth = imageParams.isHalf ? 2 : 4; // only support half or float

  // Map RGB(A) or XYZ buffers to correct channel order, also support R and RA
  auto channelMap = std::vector<int>(header.num_channels, -1);
  std::vector<char> channelNames = {'R','G','B','A','X','Y','Z'};
  const EXRChannelInfo *channels = header.channels;
  int ch = 0;
  for (const char name : channelNames) {
    for (size_t c = 0; c < header.num_channels; c++) {
      // Only set up to 4 channels, file may contain more of the named above
      if (ch < MaxChannels && channels[c].name[0] == name) {
        channelMap[c] = ch++;
        break;
      }
    }
  }

  // Ensure all mapped channel pixel types match
  int type = header.pixel_types[channelMap[0]];
  for (int i = 1; i < imageParams.components; i++) {
    if (type != header.pixel_types[channelMap[i]]) {
      std::cerr << "#osp:sg:EXR channel pixel types are inconsistent : " << cFN
                << std::endl;
      FreeEXRHeader(&header);
      FreeEXRImage(&image);
      return;
    }
  }

  if (imageParams.isHalf)
    loadTexture_EXR_interleaveImage<short>(
        header.num_channels, image.images, channelMap);
  else
    loadTexture_EXR_interleaveImage<float>(
        header.num_channels, image.images, channelMap);

  FreeEXRHeader(&header);
  FreeEXRImage(&image);

  if (!texelData.get()) {
    std::cerr << "#osp:sg: EXR failed to load texture '" << fileName << "'"
              << std::endl;
  }
}

//
// TIFF (via tinyDNG)
//
void Texture2D::loadTexture_TIFF(const std::string &fileName)
{
  std::string warn, err;
  std::vector<tinydng::DNGImage> images;

  // Loads all images(IFD) in the DNG file to `images` array.
  std::vector<tinydng::FieldInfo> custom_field_lists;
  bool ret = tinydng::LoadDNG(
      fileName.c_str(), custom_field_lists, &images, &warn, &err);

  if (!warn.empty()) {
    std::cout << "Warn: " << warn << std::endl;
  }

  if (!err.empty()) {
    std::cerr << "Err: " << err << std::endl;
  }

  if (ret) {
    if (images.size() > 1)
      std::cerr << "TIFF " << fileName
                << " contains multiple images, only loading first one"
                << std::endl;

    const tinydng::DNGImage &image = images[0];
    imageParams.size = vec2ul(image.width, image.height);
    imageParams.components = image.samples_per_pixel;
    imageParams.depth = image.bits_per_sample >> 3;
    size_t size = totalImageSize() * imageParams.depth;

    std::shared_ptr<uint8_t> data(
        new uint8_t[size], std::default_delete<uint8_t[]>());
    std::memcpy(data.get(), image.data.data(), size);

    // Move shared_ptr ownership
    texelData = data;
  }

  if (!texelData.get()) {
    std::cerr << "#osp:sg: TIFF failed to load texture '" << fileName << "'"
              << std::endl;
  }
}

//
// STBi
//
void Texture2D::loadTexture_STBi(const std::string &fileName)
{
  const bool isHDR = stbi_is_hdr(fileName.c_str());
  const bool is16b = stbi_is_16_bit(fileName.c_str());

  void *texels{nullptr};
  int width, height;
  if (isHDR)
    texels = (void *)stbi_loadf(
        fileName.c_str(), &width, &height, &imageParams.components, 0);
  else if (is16b)
    texels = (void *)stbi_load_16(
        fileName.c_str(), &width, &height, &imageParams.components, 0);
  else
    texels = (void *)stbi_load(
        fileName.c_str(), &width, &height, &imageParams.components, 0);

  imageParams.size = vec2ul(width, height);
  imageParams.depth = isHDR ? 4 : is16b ? 2 : 1;

  if (texels) {
    // XXX stbi uses malloc/free override these with our alignedMalloc/Free
    // (and implement a realloc?) to prevent this memcpy?
    size_t size = totalImageSize() * imageParams.depth;
    std::shared_ptr<uint8_t> data(
        new uint8_t[size], std::default_delete<uint8_t[]>());
    std::memcpy(data.get(), texels, size);
    texelData = data;
    stbi_image_free(texels);
  }

  if (!texelData.get()) {
    std::cerr << "#osp:sg: STB_image failed to load texture '" + fileName + "'"
              << std::endl;
  }
}
#endif

// Texture2D UDIM ///////////////////////////////////////////////////////////

// Check texture filename for udim pattern.  Then check that each tile file
// exists.  Image size/format will be checked on load.  This is a quick check.
bool Texture2D::checkForUDIM(FileName filename)
{
  std::string fullName = filename.str();

  // Quick return if texture is already UDIM
  if (hasUDIM())
    return true;

  // See if base tile "1001" is in the filename. If not, it's not a UDIM.
  auto found = fullName.rfind("1001");
  if (found == std::string::npos)
    return false;

  if (filename.canonical() == "")
    std::cerr << "unable to find full path for UDIM file: " << filename
              << std::endl;

  // Make sure base file even exists
  std::ifstream f(fullName.c_str());
  if (!f.good())
    return false;

  // Strip off the "1001" and continue searching for other tiles
  // by checking existing files of the correct pattern.
  // This will work for most any consistent *1001* naming scheme.
  // pattern:  lFileName<tileNum>rFileName
  std::string lFileName = fullName.substr(0, found);
  std::string rFileName = fullName.substr(found + 4);

  int vmax = 0;
  int umax = 0;
  for (int v = 1; v <= 10; v++)
    for (int u = 1; u <= 10; u++) {
      std::string tileNum = std::to_string(1000 + (v - 1) * 10 + u);
      std::string checkName = lFileName + tileNum + rFileName;
      std::ifstream f(checkName.c_str());
      if (f.good()) {
        udimTile tile(checkName, vec2i(u - 1, v - 1));
        udim_params.tiles.push_back(tile);
        vmax = std::max(vmax, v);
        umax = std::max(umax, u);
      }
    }

  if (umax > 1) {
    udim_params.dims.y = vmax;
    udim_params.dims.x = vmax > 1 ? 10 : umax;
  }

  return umax > 1;
}

void Texture2D::loadUDIM_tiles(const FileName &fileName)
{
  if (udim_params.tiles.size() < 2) {
    std::cerr << "#osp:sg: loadUDIM_tiles: not a udim atlas" << std::endl;
    return;
  }

  // Create two temporary working nodes
  // work contains each loaded tile and builds a texture atlas into main
  auto atlas = &createChildAs<Texture2D>("udim_main", "texture_2d");
  auto work = &createChildAs<Texture2D>("udim_work", "texture_2d");

  // Use the same params as parent texture
  // but, mark as "loading" textures to skip re-checking udim tiles
  work->imageParams = imageParams;
  work->samplerParams = samplerParams;
  // Don't flip the work tiles, the atlas will be flipped if necessary.
  work->flip = false;
  work->udim_params.loading = true;

  // Load the first tile to establish tile parameters
  auto tile = udim_params.tiles.front();
  work->texelData = nullptr;
  work->load(tile.first);
  udim_params.tiles.pop_front();

  auto tileSize = work->imageParams.size;
  auto tileDepth = work->imageParams.depth;
  auto tileComponents = work->imageParams.components;
  auto texelSize = tileDepth * tileComponents;
  auto tileStride = tileSize.x * texelSize;

  // Allocate space large enough to hold all tiles (all tiles guaranteed to be
  // of equal size and format)
  atlas->imageParams = work->imageParams;
  atlas->samplerParams = work->samplerParams;
  // If requested, flip the entire atlas after tiles have been assembled.
  atlas->flip = flip;
  atlas->udim_params = work->udim_params;
  atlas->imageParams.size *= udim_params.dims;
  std::shared_ptr<uint8_t> data(
      new uint8_t[atlas->imageParams.size.product() * texelSize],
      std::default_delete<uint8_t[]>());
  atlas->texelData = data;
  auto atlasStride = atlas->imageParams.size.x * texelSize;

  // Lambda to copy work tile into atlas
  auto CopyTile = [&](vec2i origin) {
    uint8_t *dest = (uint8_t *)data.get() + origin.y * tileSize.y * atlasStride
        + origin.x * tileStride;
    uint8_t *src = (uint8_t *)work->texelData.get();
    for (int y = 0; y < tileSize.y; y++)
      std::memcpy(dest + y * atlasStride, src + y * tileStride, tileStride);
  };

  CopyTile(tile.second);

  // Load the remaining tiles into the atlas
  for (const auto &tile : udim_params.tiles) {
    work->texelData = nullptr;
    work->load(tile.first);
    // XXX TODO, allow different size/format tiles?
    // This would require pre-loading all tiles and setting atlas to multiple
    // of the largest size, then scaling all tiles into the atlas.
    if (work->imageParams.size != tileSize
        || work->imageParams.depth != tileDepth
        || work->imageParams.components != tileComponents) {
      std::cerr << "#osp:sg: udim tile size or format doesn't match, skipping: "
                << tile.first << std::endl;
      continue;
    }

    CopyTile(tile.second);

    // Don't keep tiles in the texture cache
    textureCache.erase(tile.first);
  }

  // Copy atlas back to parent
  imageParams = atlas->imageParams;
  samplerParams = atlas->samplerParams;
  texelData = atlas->texelData;
  for (auto &c : atlas->children())
    add(c.second);

  // Remove the temporary working nodes
  remove("udim_work");
  remove("udim_main");
}

// Texture2D public methods /////////////////////////////////////////////////

void Texture2D::flipImage()
{
  uint32_t width = imageParams.size.x;
  uint32_t height = imageParams.size.y;
  int stride = width * imageParams.depth * imageParams.components;
  tasking::parallel_for(height >> 1, [&](uint32_t row) {
    uint8_t *temp = (uint8_t *)malloc(stride);
    uint8_t *src = (uint8_t *)texelData.get() + stride * row;
    uint8_t *dst = (uint8_t *)texelData.get() + (height - 1 - row) * stride;
    memcpy(temp, dst, stride);
    memcpy(dst, src, stride);
    memcpy(src, temp, stride);
    free(temp);
  });
  isFlipped ^= true;
}

bool Texture2D::load(const FileName &_fileName, const void *memory)
{
  bool success = false;
  // Not a true filename in the case memory != nullptr (since texture is
  // already in memory), but a unique name for the texture cache.
  fileName = _fileName;

  // Check the cache before creating a new texture
  if (!reload && textureCache.find(fileName) != textureCache.end()) {
    std::shared_ptr<Texture2D> cache = textureCache[fileName].lock();
    if (cache) {
      // Adopt cache image parameters, including udim (if applicable)
      imageParams = cache->imageParams;
      udim_params = cache->udim_params;
      // Copy shared_ptr ownership
      texelData = cache->texelData;
      // the same texelData is being reused, so use the existing isFlipped state
      isFlipped = cache->isFlipped;

      // If sampler parameters match, just add existing children
      if (cache->hasChild("data")) {
        if (samplerParamsMatch(cache->samplerParams)) {
          // Add all children of cache, and copy sampler paraameters
          samplerParams = cache->samplerParams;
          for (const auto &param : cache->children())
            add(param.second);
          success = true;
        }
      }
    }
  } 

  // If texelData = nullptr then a cached image was not found, proceed with load
  if (texelData == nullptr) {
    if (memory) {
      size_t size = totalImageSize() * imageParams.depth;
      std::shared_ptr<uint8_t> data(
          new uint8_t[size], std::default_delete<uint8_t[]>());
      std::memcpy(data.get(), memory, size);
      // Move shared_ptr ownership
      texelData = data;

    } else {
      // Check if fileName indicates a UDIM atlas and load tiles
      if (!udim_params.loading && checkForUDIM(fileName))
        loadUDIM_tiles(fileName);
      else {
        // Check that fileName exist 
        std::ifstream f(fileName);
        if (!f.good()) {
          std::cerr << "#osp:sg: Texture file error, check '" + fileName + "'"
                    << std::endl;
        } else {
#ifdef USE_OPENIMAGEIO
          loadTexture_OIIO(fileName);
#else
          if (_fileName.ext() == "exr")
            loadTexture_EXR(fileName);
          else if (_fileName.ext() == "tif" || _fileName.ext() == "tiff")
            loadTexture_TIFF(fileName);
          else if (_fileName.ext() == "pfm")
            loadTexture_PFM(fileName);
          else
            loadTexture_STBi(fileName);
#endif
        }
      }
    }
  }

  // If success = true, then cached texture already filled in the children.
  if (!success && texelData.get()) {
    // If needed, flip the image before creating the data object
    if (flip && !isFlipped)
      flipImage();

    // Create the OSPRay "data" node that holds the image data
    createDataNode();

    // If the load was successful, populate children
    if (hasChild("data")) {
      child("data").setSGNoUI();

      // If not using all channels, set used components to 1 for texture format
      auto ospTexFormat = osprayTextureFormat(
          samplerParams.channel < 4 ? 1 : imageParams.components);
      auto texFilter = samplerParams.nearestFilter ? OSP_TEXTURE_FILTER_NEAREST
                                                   : OSP_TEXTURE_FILTER_LINEAR;

      createChild("format",
          "OSPTextureFormat",
          "0 = OSP_TEXTURE_RGBA8\n"
          "1 = OSP_TEXTURE_SRGBA\n"
          "2 = OSP_TEXTURE_RGBA32F\n"
          "3 = OSP_TEXTURE_RGB8\n"
          "4 = OSP_TEXTURE_SRGB\n"
          "5 = OSP_TEXTURE_RGB32F\n"
          "6 = OSP_TEXTURE_R8\n"
          "7 = OSP_TEXTURE_R32F\n"
          "8 = OSP_TEXTURE_L8\n"
          "9 = OSP_TEXTURE_RA8\n"
          "10 = OSP_TEXTURE_LA8\n"
          "11 = OSP_TEXTURE_RGBA16\n"
          "12 = OSP_TEXTURE_RGB16\n"
          "13 = OSP_TEXTURE_RA16\n"
          "14 = OSP_TEXTURE_R16\n"
          "15 = OSP_TEXTURE_RA32F\n"
          "16 = OSP_TEXTURE_RGBA16F\n"
          "17 = OSP_TEXTURE_RGB16F\n"
          "18 = OSP_TEXTURE_RA16F\n"
          "19 = OSP_TEXTURE_R16F\n",
          ospTexFormat);

      createChild("filter",
          "OSPTextureFilter",
          " 0 = OSP_TEXTURE_FILTER_LINEAR\n"
          " 1 = OSP_TEXTURE_FILTER_NEAREST\n",
          texFilter);

      // Note: Not possible to create node of type vec_t<OSPTextureWrapMode, 2>
      // OSPRay treats the param as vec2ui
      createChild("wrapMode",
          "vec2ui",
          " S/T Texture Wrap modes\n"
          " 0 = OSP_TEXTURE_WRAP_REPEAT (default)\n"
          " 1 = OSP_TEXTURE_WRAP_MIRRORED_REPEAT\n"
          " 2 = OSP_TEXTURE_WRAP_CLAMP_TO_EDGE\n",
          vec2ui(samplerParams.texWrapMode));

      createChild("filename", "filename", fileName).setSGOnly();
      createChild("isFlipped", "bool", flip).setSGOnly();

#if 1 // XXX Running MPI, simply changing the texture data is not enough to
      // trigger the flip, therefore do not expose an option for the user
      // to change it.  Find a lightweight means to signal MPI to update
      // texture data to all ranks.  (note: The below in preCommit doesn't work)
      child("isFlipped").setReadOnly();
#endif

      child("format").setMinMax(OSP_TEXTURE_RGBA8, OSP_TEXTURE_R16F);
      child("filter").setMinMax(
          OSP_TEXTURE_FILTER_LINEAR, OSP_TEXTURE_FILTER_NEAREST);
      child("wrapMode")
          .setMinMax(uint32_t(OSP_TEXTURE_WRAP_REPEAT),
              uint32_t(OSP_TEXTURE_WRAP_CLAMP_TO_EDGE));

      // Add this texture to the cache
      textureCache[fileName] = this->nodeAs<Texture2D>();
      success = true;
    } else
      std::cerr << "Failed texture " << fileName << std::endl;
  }

  return success;
}

// Texture2D definitions ////////////////////////////////////////////////////

Texture2D::Texture2D() : Texture("texture2d") {}
Texture2D::~Texture2D()
{
  textureCache.erase(fileName);
}

void Texture2D::preCommit()
{
#if 0 // XXX this does not work for MPI
  // If needed, flip the image
  if (isFlipped != child("isFlipped").valueAs<bool>()) {
    flipImage();
    // Simply changing memory contents will not trigger OSPRay to update texture
    // data, especially running MPI offload.  Remove and re-add the OSPData node
    auto &texture = valueAs<cpp::Texture>();
    texture.commit();
  }
#endif

  std::string guiFilename = child("filename").valueAs<std::string>();
  if (fileName != guiFilename) {
    // Reset to defaults and load new texture
    texelData = nullptr;
    flip = true;
    isFlipped = false;
    udim_params = {};
    textureCache.erase(fileName);
    load(guiFilename);
  }

  // make sure to call base-class precommit
  Texture::preCommit();
}

void Texture2D::postCommit()
{
  Texture::postCommit();
}

OSP_REGISTER_SG_NODE_NAME(Texture2D, texture_2d);

std::map<std::string, std::weak_ptr<Texture2D>> Texture2D::textureCache;

} // namespace sg
} // namespace ospray
