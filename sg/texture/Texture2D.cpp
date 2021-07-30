// Copyright 2009-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Texture2D.h"
#include <sstream>
#include "rkcommon/memory/malloc.h"

#include "stb_image.h"

namespace ospray {
  namespace sg {

  // static helper functions //////////////////////////////////////////////////
 
  //
  // Generic
  //
  // Create the childData node given all other texture params
  inline void Texture2D::createDataNode()
  {
    if (params.depth == 1)
      createDataNodeType_internal<uint8_t>();
    else if (params.depth == 2)
      createDataNodeType_internal<uint16_t>();
    else if (params.depth == 4)
      createDataNodeType_internal<float>();
    else
      std::cerr << "#osp:sg: INVALID Texture depth " << params.depth
                << std::endl;
  }

  template <typename T>
  void Texture2D::createDataNodeType_internal()
  {
    if (params.components == 1)
      createDataNodeVec_internal<T>();
    else if (params.components == 2)
      createDataNodeVec_internal<T, 2>();
    else if (params.components == 3)
      createDataNodeVec_internal<T, 3>();
    else if (params.components == 4)
      createDataNodeVec_internal<T, 4>();
    else
      std::cerr << "#osp:sg: INVALID number of texture components "
                << params.components << std::endl;
  }
  template <typename T, int N>
  void Texture2D::createDataNodeVec_internal()
  {
    using vecT = vec_t<T, N>;
    // If texture doesn't use all channels(4), setup a strided-data access
    if (params.colorChannel < 4) {
      createChildData("data",
          params.size, // numItems
          sizeof(vecT) * vec2ul(1, params.size.x), // byteStride
          (T *)texelData.get() + params.colorChannel, true);
    } else // RGBA
      createChildData(
          "data", params.size, vec2ul(0, 0), (vecT *)texelData.get(), true);
  }
  template <typename T>
  void Texture2D::createDataNodeVec_internal()
  {
    createChildData(
        "data", params.size, vec2ul(0, 0), (T *)texelData.get(), true);
  }

  OSPTextureFormat Texture2D::osprayTextureFormat()
  {
    if (params.depth == 1) {
      if (params.components == 1)
        return params.preferLinear ? OSP_TEXTURE_R8 : OSP_TEXTURE_L8;
      if (params.components == 2)
        return params.preferLinear ? OSP_TEXTURE_RA8 : OSP_TEXTURE_LA8;
      if (params.components == 3)
        return params.preferLinear ? OSP_TEXTURE_RGB8 : OSP_TEXTURE_SRGB;
      if (params.components == 4)
        return params.preferLinear ? OSP_TEXTURE_RGBA8 : OSP_TEXTURE_SRGBA;
    } else if (params.depth == 2) {
      if (params.components == 1)
        return OSP_TEXTURE_R16;
      if (params.components == 2)
        return OSP_TEXTURE_RA16;
      if (params.components == 3)
        return OSP_TEXTURE_RGB16;
      if (params.components == 4)
        return OSP_TEXTURE_RGBA16;
    } else if (params.depth == 4) {
      if (params.components == 1)
        return OSP_TEXTURE_R32F;
      if (params.components == 3)
        return OSP_TEXTURE_RGB32F;
      if (params.components == 4)
        return OSP_TEXTURE_RGBA32F;
    }

    std::cerr << "#osp:sg: INVALID format " << params.depth << ":"
              << params.components << std::endl;
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
    const auto typeDesc = TypeDescFromC<T>::value();

    std::shared_ptr<void> data(
        new T[params.size.product() * params.components]);
    T *start = (T *)data.get()
        + (params.flip ? (params.size.y - 1) * params.size.x * params.components
                       : 0);
    const long int stride =
        (params.flip ? -1 : 1) * params.size.x * sizeof(T) * params.components;

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

      params.size = vec2ul(spec.width, spec.height);
      params.components = spec.nchannels;
      params.depth = spec.format.size();

      // This has only been seen on the Bentley .exr 16-bit textures so far.
      // But, it is a more general problem.
      // For now, just load 16-bit texture as uchar.
      // XXX 16-bit and 32-bit images might be sampled as sRGB?
      // Isn't colorspace separate from bit depth, 16/32-bit sRGB should exist?
      // Shouldn't OSPRay have formats for these?
      if (params.depth == 2 && !params.preferLinear)
        params.depth = 1;

      if (params.depth == 1)
        loadTexture_OIIO_readFile<uint8_t>(in);
      else if (params.depth == 2)
        loadTexture_OIIO_readFile<uint16_t>(in);
      else if (params.depth == 4)
        loadTexture_OIIO_readFile<float>(in);
      else
        std::cerr << "#osp:sg: INVALID Texture depth " << params.depth
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
    size_t size = params.size.product() * params.components;
    std::shared_ptr<void> data (new float[size]);
    const size_t dataSize = sizeof(size)*sizeof(float);

    int rc = fread(data.get(), dataSize, 1, file);
    if (rc) {
      // Scale texels by scale factor
      float *texels = (float *)data.get();
      for (size_t i = 0; i < params.size.product(); i++)
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

      params.components = 3;
      if (format[1] == 'f') {
        params.components = 1;
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

      // read scale factor/endiannes
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
      params.size = vec2ul(width, height);
      params.depth = 4; // pfm is always float

      loadTexture_PFM_readFile(file, scaleFactor);

      if (!texelData.get())
        std::cerr << "#osp:sg: INVALID FORMAT PFM " << params.components
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
  // STBi
  //
  void Texture2D::loadTexture_STBi(const std::string &fileName)
  {
    stbi_set_flip_vertically_on_load(params.flip);

    const bool isHDR = stbi_is_hdr(fileName.c_str());
    const bool is16b = stbi_is_16_bit(fileName.c_str());

    void *texels{nullptr};
    int width, height;
    if (isHDR)
      texels = (void *)stbi_loadf(
          fileName.c_str(), &width, &height, &params.components, 0);
    else if (is16b)
      texels = (void *)stbi_load_16(
          fileName.c_str(), &width, &height, &params.components, 0);
    else
      texels = (void *)stbi_load(
          fileName.c_str(), &width, &height, &params.components, 0);

    params.size = vec2ul(width, height);
    params.depth = isHDR ? 4 : is16b ? 2 : 1;

    if (texels) {
      // XXX stbi uses malloc/free override these with our alignedMalloc/Free
      // (and implement a realloc?) to prevent this memcpy?
      size_t size = params.size.product() * params.components * params.depth;
      std::shared_ptr<void> data (new uint8_t[size]);
      memcpy(data.get(), texels, size);
      texelData = data;
      stbi_image_free(texels);
    }

    if (!texelData.get()) {
      std::cerr << "#osp:sg: STB_image failed to load texture '" + fileName
              + "'"
                << std::endl;
      std::cerr << "#osp:sg: Rebuilding OSPRay Studio with OpenImageIO "
                << "support may fix this error." << std::endl;
    }
  }
#endif

  void Texture2D::loadUDIM_tiles(const FileName &fileName)
  {
    std::cout << "!!!!!!!! texture load UDIM !!!!!!!" << std::endl; // XXX
    std::string fullName = fileName.str();

    // Scan for extent of tiles.
    // create a separate node for each file,
    // build all texture tiles into the original node and hand that back
    // delete all tiles

#ifdef USE_OPENIMAGEIO
    loadTexture_OIIO(fileName);
#else
    if (fileName.ext() == "pfm")
      loadTexture_PFM(fileName);
    else
      loadTexture_STBi(fileName);
#endif

#if 0
  // Find the position of the tile number in the fileName
  auto found = fullName.rfind("1001");
  if (found == std::string::npos) {
    std::cerr << "!!! BAD NEWS.... not a UDIM !!!" << std::emdl;
    return;
  }

#if 1 // extract into a find_udim_files() return vector of valid files
  // Strip off the "1001" and continue searching for other tiles
  // by checking existing files of the correct names.
  // This will work for most any *1001* naming scheme.
  std::string lFileName = fullName.substr(0, found);
  std::string rFileName = fullName.substr(found + 4);

  std::cout << "isUDIM... pattern " << lFileName << "*" << rFileName
            << std::endl;

  int vdim = 0;
  int udim = 0;
  for (int v = 0; v < 10 && udim == 0; v++)
    for (int u = 0; u < 10; u++) {
      std::string tileNum = std::to_string(1001 + v * 10 + u);
      std::string checkName = lFileName + tileNum + rFileName;
      std::cout << "isUDIM... " << checkName << std::endl;
      std::ifstream f(checkName.c_str());
      // Exit loop on first missing tile-filename.
      if (!f.good()) {
        std::cout << "isUDIM... missing " << checkName << std::endl;
        vdim = v;
        udim = v == 0 ? u : 10;
        break;
      }
      std::cout << "isUDIM... found " << checkName << std::endl;
    }
#endif

  if (vdim > 0 || udim > 1)
    isUDIM = true;

  std::cout << "isUDIM... (" << isUDIM << ") " << vdim << ":" << udim
            << std::endl;

  return isUDIM;
#endif
  }

  void Texture2D::load(const FileName &_fileName,
      const bool _preferLinear,
      const bool _nearestFilter,
      const int _colorChannel)
  {
    fileName = _fileName;

    // Check the cache before creating a new texture
    // XXX doesn't yet work to cache textures using different channels
    if (_colorChannel == 4
        && textureCache.find(fileName) != textureCache.end()) {
      std::shared_ptr<Texture2D> cache = textureCache[fileName].lock();
      if (cache) {
        params = cache->params;
        // Copy shared_ptr ownership
        texelData = cache->texelData;
      }
    } else {
      // Check whether this is a UDIM tile set
      if (checkUDIM(fileName)) {
        loadUDIM_tiles(fileName);
      } else {
#ifdef USE_OPENIMAGEIO
        loadTexture_OIIO(fileName);
#else
        if (_fileName.ext() == "pfm")
          loadTexture_PFM(fileName);
        else
          loadTexture_STBi(fileName);
#endif
      }

      // Add this texture to the cache
      if (texelData.get()) {
        textureCache[fileName] = this->nodeAs<Texture2D>();
      }
    }

    if (texelData.get()) {
      params.preferLinear = _preferLinear;
      params.nearestFilter = _nearestFilter;
      params.colorChannel = _colorChannel;
    }
  }

  void Texture2D::load(void *memory,
      const bool _preferLinear,
      const bool _nearestFilter,
      const int _colorChannel)
  {
    std::stringstream ss;
    ss << "memory: " << std::hex << memory;
    fileName = ss.str();

    // Check the cache before creating a new texture
    // XXX doesn't yet work to cache textures using different channels
    if (_colorChannel == 4
        && textureCache.find(fileName) != textureCache.end()) {
      std::shared_ptr<Texture2D> cache = textureCache[fileName].lock();
      if (cache) {
        params = cache->params;
        // Copy shared_ptr ownership
        texelData = cache->texelData;
      }
    } else {
      if (memory) {
        size_t size = params.size.product() * params.components * params.depth;
        std::shared_ptr<void> data (new uint8_t[size]);
        memcpy(data.get(), memory, size);
        // Move shared_ptr ownership
        texelData = data;

        // Add this texture to the cache
        textureCache[fileName] = this->nodeAs<Texture2D>();
      }
    }

    if (texelData.get()) {
      params.preferLinear = _preferLinear;
      params.nearestFilter = _nearestFilter;
      params.colorChannel = _colorChannel;
    }
  }

  // Texture2D definitions ////////////////////////////////////////////////////

  Texture2D::Texture2D() : Texture("texture2d") {}
  Texture2D::~Texture2D()
  {
    auto num = textureCache.erase(fileName);
  }

  void Texture2D::preCommit()
  {
    if (texelData.get())
      createDataNode();

    // If the load was successful, populate children
    if (hasChild("data")) {
      child("data").setSGNoUI();

      // If not using all channels, set used components to 1 for texture format
      if (params.colorChannel < 4)
        params.components = 1;
      auto ospTexFormat = osprayTextureFormat();
      auto texFilter = params.nearestFilter ? OSP_TEXTURE_FILTER_NEAREST
        : OSP_TEXTURE_FILTER_BILINEAR;

      createChild("format", "int", (int)ospTexFormat);
      createChild("filter", "int", (int)texFilter);

      createChild("filename", "string", fileName);
      child("filename").setSGOnly();

      child("format").setMinMax((int)OSP_TEXTURE_RGBA8, (int)OSP_TEXTURE_R16);
      child("filter").setMinMax(
          (int)OSP_TEXTURE_FILTER_BILINEAR, (int)OSP_TEXTURE_FILTER_NEAREST);
    } else
      std::cout << "Failed texture " << fileName << std::endl;

    // make sure to call base-class precommit
    Texture::preCommit();
  }

  void Texture2D::postCommit()
  {
    Texture::postCommit();
  }

  OSP_REGISTER_SG_NODE_NAME(Texture2D, texture_2d);

  std::map<std::string, std::weak_ptr<Texture2D>> Texture2D::textureCache;

  }  // namespace sg
} // namespace ospray
