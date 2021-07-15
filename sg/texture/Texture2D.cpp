// Copyright 2009-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Texture2D.h"
#include <sstream>
#include "stb_image.h"

namespace ospray {
  namespace sg {

  // static helper functions //////////////////////////////////////////////////
 
  //
  // Generic
  //
  template <typename T>
  void Texture2D::createDataNodeType_internal()
  {
    if (components == 1)
      createDataNodeVec_internal<T>();
    else if (components == 2)
      createDataNodeVec_internal<T, 2>();
    else if (components == 3)
      createDataNodeVec_internal<T, 3>();
    else if (components == 4)
      createDataNodeVec_internal<T, 4>();
    else
      std::cerr << "#osp:sg: INVALID number of texture components "
                << components << std::endl;
  }
  template <typename T, int N>
  void Texture2D::createDataNodeVec_internal()
  {
    using vecT = vec_t<T, N>;
    // If texture doesn't use all channels(4), setup a strided-data access
    if (colorChannel < 4) {
      createChildData("data",
          size, // numItems
          sizeof(vecT) * vec2ul(1, size.x), // byteStride
          (T *)texelData + colorChannel);
    } else // RGBA
      createChildData("data", size, vec2ul(0, 0), (vecT *)texelData);
  }
  template <typename T>
  void Texture2D::createDataNodeVec_internal()
  {
    createChildData("data", size, vec2ul(0, 0), (T *)texelData);
  }

  OSPTextureFormat Texture2D::osprayTextureFormat()
  {
    if (depth == 1) {
      if (components == 1)
        return preferLinear ? OSP_TEXTURE_R8 : OSP_TEXTURE_L8;
      if (components == 2)
        return preferLinear ? OSP_TEXTURE_RA8 : OSP_TEXTURE_LA8;
      if (components == 3)
        return preferLinear ? OSP_TEXTURE_RGB8 : OSP_TEXTURE_SRGB;
      if (components == 4)
        return preferLinear ? OSP_TEXTURE_RGBA8 : OSP_TEXTURE_SRGBA;
    } else if (depth == 2) {
      if (components == 1)
        return OSP_TEXTURE_R16;
      if (components == 2)
        return OSP_TEXTURE_RA16;
      if (components == 3)
        return OSP_TEXTURE_RGB16;
      if (components == 4)
        return OSP_TEXTURE_RGBA16;
    } else if (depth == 4) {
      if (components == 1)
        return OSP_TEXTURE_R32F;
      if (components == 3)
        return OSP_TEXTURE_RGB32F;
      if (components == 4)
        return OSP_TEXTURE_RGBA32F;
    }

    std::cerr << "#osp:sg: INVALID format " << depth << ":" << components
      << std::endl;
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

    std::vector<T> data(size.x * size.y * components);
    T *start = data.data() + (flip ? (size.y - 1) * size.x * components : 0);
    const size_t stride = (flip ? -1 : 1) * size.x * sizeof(T) * components;

    bool success =
        in->read_image(typeDesc, start, AutoStride, stride, AutoStride);
    if (success) {
      texelData = (void *)data.data();
      createDataNode();
    }
  }

  void Texture2D::loadTexture_OIIO(const std::string &fileName)
  {
    auto in = ImageInput::open(fileName.c_str());
    if (in) {
      const ImageSpec &spec = in->spec();
      const auto typeDesc = spec.format.elementtype();

      size = vec2ul(spec.width, spec.height);
      components = spec.nchannels;
      depth = spec.format.size();

      if (depth == 1) {
        loadTexture_OIIO_readFile<uint8_t>(in);
      } else if (depth == 2) {
        loadTexture_OIIO_readFile<uint16_t>(in);
      } else if (depth == 4) {
        loadTexture_OIIO_readFile<float>(in);
      } else
        std::cerr << "#osp:sg: INVALID Texture depth " << depth << std::endl;

      in->close();
#if OIIO_VERSION < 10903 && OIIO_VERSION > 10603
      ImageInput::destroy(in);
#endif
    }

    if (!hasChild("data")) {
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
    std::vector<float> data(size.x * size.y * components);
    const size_t dataSize = data.size() * sizeof(float);

    int rc = fread(data.data(), dataSize, 1, file);
    if (rc) {
      // Scale texels by scale factor
      std::transform(data.begin(), data.end(), data.begin(), [&](float v) {
        return v * scaleFactor;
      });

      texelData = (void *)data.data();
      createDataNode();
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

      components = 3;
      if (format[1] == 'f') {
        components = 1;
      }

      // read width and height
      int width = -1;
      int height = -1;
      rc = fscanf(file, "%i %i\n", &width, &height);
      if (rc != 2) {
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
      size = vec2ul(width, height);
      depth = 4; // pfm is always float

      loadTexture_PFM_readFile(file, scaleFactor);

      if (!hasChild("data"))
        std::cerr << "#osp:sg: INVALID FORMAT PFM " << components << std::endl;

    } catch (const std::runtime_error &e) {
      std::cerr << "#osp:sg: INVALID PFM" << std::endl;
      std::cerr << e.what() << std::endl;
    }

    if (file)
      fclose(file);

    if (!hasChild("data")) {
      std::cerr << "#osp:sg: PFM failed to load texture '" << fileName << "'"
        << std::endl;
    }
  }

  //
  // STBi
  //
  void Texture2D::loadTexture_STBi(const std::string &fileName)
  {
    stbi_set_flip_vertically_on_load(flip);

    const bool isHDR = stbi_is_hdr(fileName.c_str());
    const bool is16b = stbi_is_16_bit(fileName.c_str());

    void *pixels = nullptr;
    int width, height;
    if (isHDR)
      pixels =
          (void *)stbi_loadf(fileName.c_str(), &width, &height, &components, 0);
    else if (is16b)
      pixels = (void *)stbi_load_16(
          fileName.c_str(), &width, &height, &components, 0);
    else
      pixels =
          (void *)stbi_load(fileName.c_str(), &width, &height, &components, 0);

    size = vec2ul(width, height);
    depth = isHDR ? 4 : is16b ? 2 : 1;

    if (pixels) {
      texelData = pixels;
      createDataNode();
      stbi_image_free(pixels); // XXX allow Texture2D class to manage lifetime
    }

    if (!hasChild("data")) {
      std::cerr << "#osp:sg: STB_image failed to load texture '" + fileName
              + "'"
                << std::endl;
      std::cerr << "#osp:sg: Rebuilding OSPRay Studio with OpenImageIO "
                << "support may fix this error." << std::endl;
    }
  }
#endif

  // Texture2D definitions ////////////////////////////////////////////////////

  Texture2D::Texture2D() : Texture("texture2d") {}
  Texture2D::~Texture2D()
  {
    textureCache.erase(fileName);
  }

  void Texture2D::load(const FileName &_fileName,
      const bool _preferLinear,
      const bool _nearestFilter,
      const int _colorChannel)
  {
    fileName = _fileName;
    const std::string displayName = _fileName.base();

    bool isUDIM = checkUDIM(fileName);
    // Check whether this is a UDIM tile set
    if (isUDIM) {
      std::cout << "!!!!!!!! texture load UDIM !!!!!!!" << std::endl;
    }

    std::shared_ptr<Texture2D> newTexNode = nullptr;

    // XXX Cache can't be used for single channel textures, yet
    // Check the cache before creating a new texture
    if (_colorChannel == 4
        && textureCache.find(fileName) != textureCache.end()) {
      newTexNode = textureCache[fileName];
    } else {
      newTexNode = createNodeAs<sg::Texture2D>(fileName, "texture_2d");

      newTexNode->preferLinear = _preferLinear;
      newTexNode->nearestFilter = _nearestFilter;
      newTexNode->colorChannel = _colorChannel;
      newTexNode->flip = flip;

#ifdef USE_OPENIMAGEIO
      newTexNode->loadTexture_OIIO(fileName);
#else
      if (_fileName.ext() == "pfm")
        newTexNode->loadTexture_PFM(fileName);
      else
        newTexNode->loadTexture_STBi(fileName);
#endif

      // If the load was successful, add texture node to cache and populate
      // object
      if (newTexNode->hasChild("data")) {
        newTexNode->child("data").setSGNoUI();

        // If not using all channels, set used components to 1
        if (_colorChannel < 4)
          newTexNode->components = 1;
        auto ospTexFormat = newTexNode->osprayTextureFormat();
        auto texFilter = _nearestFilter ? OSP_TEXTURE_FILTER_NEAREST
                                       : OSP_TEXTURE_FILTER_BILINEAR;

        newTexNode->createChild("format", "int", (int)ospTexFormat);
        newTexNode->createChild("filter", "int", (int)texFilter);

        newTexNode->createChild("filename", "string", displayName);
        newTexNode->child("filename").setSGOnly();

        newTexNode->child("format").setMinMax(
            (int)OSP_TEXTURE_RGBA8, (int)OSP_TEXTURE_R16);
        newTexNode->child("filter").setMinMax(
            (int)OSP_TEXTURE_FILTER_BILINEAR, (int)OSP_TEXTURE_FILTER_NEAREST);

        textureCache[fileName] = newTexNode;
      } else {
        // The load must have failed, don't keep the node.
        std::cout << "Failed texture " << newTexNode->name()
                  << " removing newTexNode" << std::endl;
        remove(newTexNode);
        newTexNode = nullptr;
      }
    }

    // Populate the parent node with texture parameters
    if (newTexNode) {
      size = newTexNode->size;
      components = newTexNode->components;
      depth = newTexNode->depth;
      preferLinear = _preferLinear;
      nearestFilter = _nearestFilter;
      colorChannel = _colorChannel;

      for (auto &c : newTexNode->children())
        add(c.second);
    }
  }

  void Texture2D::load(void *memory,
      const bool _preferLinear,
      const bool _nearestFilter,
      const int _colorChannel)
  {
    std::stringstream ss;
    ss << "memory: " << std::hex << memory;
    const std::string displayName = ss.str();

    std::shared_ptr<Texture2D> newTexNode =
        createNodeAs<sg::Texture2D>(fileName, "texture_2d");

    newTexNode->preferLinear = _preferLinear;
    newTexNode->nearestFilter = _nearestFilter;
    newTexNode->colorChannel = _colorChannel;
    newTexNode->flip = false;

    newTexNode->size = size;
    newTexNode->components = components;
    newTexNode->depth = depth;

    newTexNode->texelData = (void *)memory;
    newTexNode->createDataNode();

    if (newTexNode->hasChild("data")) {
      newTexNode->child("data").setSGNoUI();

      // If not using all channels, set used components to 1
      if (_colorChannel < 4)
        newTexNode->components = 1;
      auto ospTexFormat = newTexNode->osprayTextureFormat();
      auto texFilter = _nearestFilter ? OSP_TEXTURE_FILTER_NEAREST
        : OSP_TEXTURE_FILTER_BILINEAR;

      newTexNode->createChild("format", "int", (int)ospTexFormat);
      newTexNode->createChild("filter", "int", (int)texFilter);

      newTexNode->createChild("filename", "string", displayName);
      newTexNode->child("filename").setSGOnly();

      newTexNode->child("format").setMinMax(
          (int)OSP_TEXTURE_RGBA8, (int)OSP_TEXTURE_R16);
      newTexNode->child("filter").setMinMax(
          (int)OSP_TEXTURE_FILTER_BILINEAR, (int)OSP_TEXTURE_FILTER_NEAREST);

    } else {
      // The load must have failed, don't keep the node.
      std::cout << "Failed texture " << newTexNode->name()
        << " removing newTexNode" << std::endl;
      remove(newTexNode);
      newTexNode = nullptr;
    }

    // Populate the parent node with texture parameters
    if (newTexNode) {
      preferLinear = _preferLinear;
      nearestFilter = _nearestFilter;
      colorChannel = _colorChannel;

      for (auto &c : newTexNode->children())
        add(c.second);
    }
  }

  // XXX: This is where the actual load work should happen, not in load
  void Texture2D::preCommit()
  {
    Texture::preCommit();
  }

  void Texture2D::postCommit()
  {
    Texture::postCommit();
  }

  OSP_REGISTER_SG_NODE_NAME(Texture2D, texture_2d);

  std::map<std::string, std::shared_ptr<Texture2D>> Texture2D::textureCache;

  }  // namespace sg
} // namespace ospray
