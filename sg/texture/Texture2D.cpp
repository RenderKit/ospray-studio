// Copyright 2009-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "stb_image.h"

#ifdef USE_OPENIMAGEIO
#include <OpenImageIO/imageio.h>
OIIO_NAMESPACE_USING
#endif

#include "Texture2D.h"

#include "rkcommon/memory/malloc.h"

// XXX Fix texture cache

namespace ospray {
  namespace sg {

  // static helper functions //////////////////////////////////////////////////

#ifdef USE_OPENIMAGEIO
  template <typename T>
  void generateOIIOTex(std::vector<T> &imageData,
                      const size_t stride,
                      int height)
  {
      // flip image (because OSPRay's textures have the origin at the lower
      // left corner)
      unsigned char *data = (unsigned char *)imageData.data();
      for (int y = 0; y < height / 2; y++) {
        unsigned char *src  = &data[y * stride];
        unsigned char *dest = &data[(height - 1 - y) * stride];
        for (size_t x = 0; x < stride; x++)
          std::swap(src[x], dest[x]);
      }
  }

  void createOIIOTexData(NodePtr texNode,
                         const std::string &fileName,
                         bool preferLinear  = false,
                         bool nearestFilter = false)
  {
    auto in = ImageInput::open(fileName.c_str());
    if (!in) {
      std::cerr << "#osp:sg: failed to load texture ' " << fileName << "'" << std::endl;
      // reset();
    } else {
      const ImageSpec &spec = in->spec();
      vec2i size;
      int channels, depth;
      size.x                = spec.width;
      size.y                = spec.height;
      channels              = spec.nchannels;
      const bool hdr        = spec.format.size() > 1;
      depth                 = hdr ? 4 : 1;
      const size_t stride   = size.x * channels * depth;
      unsigned int dataSize = stride * size.y;

      if (depth == 1) {
        switch (channels) {
        case 1: {
          auto ospTexFormat = preferLinear ? OSP_TEXTURE_R8 : OSP_TEXTURE_L8;

          auto texFilter = (int)(nearestFilter ? OSP_TEXTURE_FILTER_NEAREST
                                               : OSP_TEXTURE_FILTER_BILINEAR);
          texNode->createChild("format", "int", (int)ospTexFormat);
          texNode->createChild("filter", "int", texFilter);
          std::vector<unsigned char> data(dataSize);
          in->read_image(TypeDesc::UINT8, data.data());

          generateOIIOTex<unsigned char>(data, stride, size.y);
          texNode->createChildData("data", data, vec2ul(size.x, size.y));
          break;
        }
        case 2: {
          auto ospTexFormat = preferLinear ? OSP_TEXTURE_RA8 : OSP_TEXTURE_LA8;
          auto texFilter    = (int)(nearestFilter ? OSP_TEXTURE_FILTER_NEAREST
                                               : OSP_TEXTURE_FILTER_BILINEAR);
          texNode->createChild("format", "int", (int)ospTexFormat);
          texNode->createChild("filter", "int", texFilter);
          std::vector<vec2uc> data(dataSize);
          in->read_image(TypeDesc::UINT8, data.data());

          generateOIIOTex<vec2uc>(data, stride, size.y);
          texNode->createChildData("data", data, vec2ul(size.x, size.y));
          break;
        }
        case 3: {
          auto ospTexFormat =
              preferLinear ? OSP_TEXTURE_RGB8 : OSP_TEXTURE_SRGB;
          auto texFilter = (int)(nearestFilter ? OSP_TEXTURE_FILTER_NEAREST
                                               : OSP_TEXTURE_FILTER_BILINEAR);
          texNode->createChild("format", "int", (int)ospTexFormat);
          texNode->createChild("filter", "int", texFilter);
          std::vector<vec3uc> data(dataSize);
          in->read_image(TypeDesc::UINT8, data.data());

          generateOIIOTex<vec3uc>(data, stride, size.y);
          texNode->createChildData("data", data, vec2ul(size.x, size.y));
          break;
        }
        case 4: {
          auto ospTexFormat =
              preferLinear ? OSP_TEXTURE_RGBA8 : OSP_TEXTURE_SRGBA;
          auto texFilter = (int)(nearestFilter ? OSP_TEXTURE_FILTER_NEAREST
                                               : OSP_TEXTURE_FILTER_BILINEAR);
          texNode->createChild("format", "int", (int)ospTexFormat);
          texNode->createChild("filter", "int", texFilter);
          std::vector<vec4uc> data(dataSize);
          in->read_image(TypeDesc::UINT8, data.data());

          generateOIIOTex<vec4uc>(data, stride, size.y);
          texNode->createChildData("data", data, vec2ul(size.x, size.y));
          break;
        }
        default:
          std::cerr << "#osp:sg: INVALID FORMAT " << depth << ":" << channels
                    << std::endl;
          break;
        }
      } else if (depth == 4) {
        switch (channels) {
        case 1: {
          auto ospTexFormat = OSP_TEXTURE_R32F;
          auto texFilter    = (int)(nearestFilter ? OSP_TEXTURE_FILTER_NEAREST
                                               : OSP_TEXTURE_FILTER_BILINEAR);
          texNode->createChild("format", "int", (int)ospTexFormat);
          texNode->createChild("filter", "int", texFilter);
          std::vector<float> data(dataSize);
          in->read_image(TypeDesc::FLOAT, data.data());

          generateOIIOTex<float>(data, stride, size.y);
          texNode->createChildData("data", data, vec2ul(size.x, size.y));
          break;
        }
        case 3: {
          auto ospTexFormat = OSP_TEXTURE_RGB32F;
          auto texFilter    = (int)(nearestFilter ? OSP_TEXTURE_FILTER_NEAREST
                                               : OSP_TEXTURE_FILTER_BILINEAR);
          texNode->createChild("format", "int", (int)ospTexFormat);
          texNode->createChild("filter", "int", texFilter);
          std::vector<vec3f> data(dataSize);
          in->read_image(TypeDesc::FLOAT, data.data());

          generateOIIOTex<vec3f>(data, stride, size.y);
          texNode->createChildData("data", data, vec2ul(size.x, size.y));
          break;
        }
        case 4: {
          auto ospTexFormat = OSP_TEXTURE_RGBA32F;
          auto texFilter    = (int)(nearestFilter ? OSP_TEXTURE_FILTER_NEAREST
                                               : OSP_TEXTURE_FILTER_BILINEAR);
          texNode->createChild("format", "int", (int)ospTexFormat);
          texNode->createChild("filter", "int", texFilter);
          std::vector<vec4f> data(dataSize);
          in->read_image(TypeDesc::FLOAT, data.data());

          generateOIIOTex<vec4f>(data, stride, size.y);
          texNode->createChildData("data", data, vec2ul(size.x, size.y));
          break;
        }
        default:
          std::cerr << "#osp:sg: INVALID FORMAT " << depth << ":" << channels
                    << std::endl;
          break;
        }
      }

      in->close();
#if OIIO_VERSION < 10903 && OIIO_VERSION > 10603
      ImageInput::destroy(in);
#endif
    }
  }
#endif

  template <typename T>
  void generatePPMTex(std::vector<T> &data,
                      unsigned int dataSize,
                      int height,
                      int width,
                      int channels,
                      const std::string filename)
  {
    unsigned char *texels = (unsigned char *)data.data();
    for (int y = 0; y < height / 2; y++)
      for (int x = 0; x < width * channels; x++) {
        unsigned int a = (y * width * channels) + x;
        unsigned int b = ((height - 1 - y) * width * channels) + x;
        if (a >= dataSize || b >= dataSize) {
          throw std::runtime_error(
              "#osp:minisg: could not parse P6 PPM file '" + filename +
              "': buffer overflow.");
        }
        std::swap(texels[a], texels[b]);
      }
  }

  template <typename T>
  void generatePFMTex(std::vector<T> &data,
                      unsigned int dataSize,
                      int height,
                      int width,
                      int numChannels,
                      float scaleFactor)
  {
    float *texels = (float *)data.data();
    for (int y = 0; y < height / 2; ++y) {
      for (int x = 0; x < width * numChannels; ++x) {
        // Scale the pixels by the scale factor
        texels[y * width * numChannels + x] =
            texels[y * width * numChannels + x] * scaleFactor;
        texels[(height - 1 - y) * width * numChannels + x] =
            texels[(height - 1 - y) * width * numChannels + x] * scaleFactor;
        std::swap(texels[y * width * numChannels + x],
                  texels[(height - 1 - y) * width * numChannels + x]);
      }
    }
  }

  template <typename T>
  void generateTex(std::vector<T> &data,
                   unsigned int dataSize,
                   int height,
                   int width,
                   int channels,
                   unsigned char *pixels,
                   bool hdr)
  {
    for (int y = 0; y < height; y++) {
      for (int x = 0; x < width; x++) {
        if (hdr) {
          const float *pixel = &((float *)pixels)[(y * width + x) * channels];
          float *dst         = &(
              (float *)data.data())[(x + (height - 1 - y) * width) * channels];
          for (int i = 0; i < channels; i++)
            *dst++ = pixel[i];
        } else {
          const unsigned char *pixel = &pixels[(y * width + x) * channels];
          unsigned char *dst =
              &((unsigned char *)
                    data.data())[((height - 1 - y) * width + x) * channels];
          for (int i = 0; i < channels; i++)
            *dst++ = pixel[i];
        }
      }
    }
  }

  // Texture2D definitions ////////////////////////////////////////////////////

  Texture2D::Texture2D() : Texture("texture2d") {}
  Texture2D::~Texture2D()
  {
    textureCache.erase(fileName);
  }

  void Texture2D::load(const FileName &_fileName,
      const bool preferLinear,
      const bool nearestFilter)
  {
    fileName = _fileName;
    const std::string baseFileName = _fileName.base();

    std::shared_ptr<Texture2D> newTexNode = nullptr;

    // Check the cache before creating a new texture
    if (textureCache.find(fileName) != textureCache.end()) {
      newTexNode = textureCache[fileName];
    } else {

      newTexNode = createNodeAs<sg::Texture2D>(fileName, "texture_2d");

#ifdef USE_OPENIMAGEIO
      createOIIOTexData(newTexNode,
          fileName,
          preferLinear,
          nearestFilter);

#else
      const std::string ext = _fileName.ext();

      if (ext == "ppm") {
        try {
          int rc, peekchar;

          // open file
          FILE *file = fopen(fileName.c_str(), "rb");
          if (!file) {
            throw std::runtime_error("#ospray_sg: could not open texture file '" +
                fileName + "'.");
          }

          const int LINESZ = 10000;
          char lineBuf[LINESZ + 1];

          // read format specifier:
          int format = 0;
          rc         = fscanf(file, "P%i\n", &format);
          if (format != 6) {
            throw std::runtime_error(
                "#ospray_sg: can currently load only binary P6 subformats for "
                "PPM texture files. "
                "Please report this bug at ospray.github.io.");
          }

          // skip all comment lines
          peekchar = getc(file);
          while (peekchar == '#') {
            auto tmp = fgets(lineBuf, LINESZ, file);
            (void)tmp;
            peekchar = getc(file);
          }
          ungetc(peekchar, file);

          // read width and height from first non-comment line
          int width = -1, height = -1;
          rc = fscanf(file, "%i %i\n", &width, &height);
          if (rc != 2) {
            throw std::runtime_error(
                "#ospray_sg: could not parse width and height in P6 PPM file "
                "'" +
                fileName +
                "'. "
                "Please report this bug at ospray.github.io, and include named "
                "file to reproduce the error.");
          }

          // skip all comment lines
          peekchar = getc(file);
          while (peekchar == '#') {
            auto tmp = fgets(lineBuf, LINESZ, file);
            (void)tmp;
            peekchar = getc(file);
          }
          ungetc(peekchar, file);

          // read maxval
          int maxVal = -1;
          rc         = fscanf(file, "%i", &maxVal);
          peekchar   = getc(file);

          if (rc != 1) {
            throw std::runtime_error(
                "#ospray_sg: could not parse maxval in P6 PPM file '" +
                fileName +
                "'. "
                "Please report this bug at ospray.github.io, and include named "
                "file to reproduce the error.");
          }

          if (maxVal != 255) {
            throw std::runtime_error(
                "#ospray_sg: could not parse P6 PPM file '" + fileName +
                "': currently supporting only maxVal=255 formats."
                "Please report this bug at ospray.github.io, and include named "
                "file to reproduce the error.");
          }

          int channels = 3;
          int depth    = 1;

          unsigned int dataSize = width * height * channels * depth;

          // flip in y, because OSPRay's textures have the origin at the lower
          // left corner

          if (channels == 1) {
            std::vector<unsigned char> data(dataSize);
            rc = fread(data.data(), dataSize, 1, file);
            generatePPMTex<unsigned char>(data, dataSize, height, width, channels, fileName);
            newTexNode->createChildData("data", data, vec2ul(width, height));
          } else if (channels == 2) {
            std::vector<vec2uc> data(dataSize);
            rc = fread(data.data(), dataSize, 1, file);
            generatePPMTex<vec2uc>(data, dataSize, height, width, channels, fileName);
            newTexNode->createChildData("data", data, vec2ul(width, height));
          } else if (channels == 3) {
            std::vector<vec3uc> data(dataSize);
            rc = fread(data.data(), dataSize, 1, file);
            generatePPMTex<vec3uc>(data, dataSize, height, width, channels, fileName);
            newTexNode->createChildData("data", data, vec2ul(width, height));
          } else if (channels == 4) {
            std::vector<vec4uc> data(dataSize);
            rc = fread(data.data(), dataSize, 1, file);
            generatePPMTex<vec4uc>(data, dataSize, height, width, channels, fileName);
            newTexNode->createChildData("data", data, vec2ul(width, height));
          }

          auto texFormat = (int)(osprayTextureFormat(depth, channels, preferLinear));
          auto texFilter = (int)(nearestFilter ? OSP_TEXTURE_FILTER_NEAREST
              : OSP_TEXTURE_FILTER_BILINEAR);
          newTexNode->createChild("format", "int", texFormat);
          newTexNode->createChild("filter", "int", texFilter);

        } catch (const std::runtime_error &e) {
          std::cerr << e.what() << std::endl;
          // reset();
        }

      } else if (ext == "pfm") {
        try {
          // Note: the PFM file specification does not support comments thus we
          // don't skip any http://netpbm.sourceforge.net/doc/pfm.html
          int rc     = 0;
          FILE *file = fopen(fileName.c_str(), "rb");
          if (!file) {
            throw std::runtime_error("#ospray_sg: could not open texture file '" +
                fileName + "'.");
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

          int numChannels = 3;
          if (format[1] == 'f') {
            numChannels = 1;
          }

          // read width and height
          int width  = -1;
          int height = -1;
          rc         = fscanf(file, "%i %i\n", &width, &height);
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
          rc                = fscanf(file, "%f\n", &scaleEndian);

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
                "#ospray_sg: scale factor/endianness in PF PFM file can not "
                "be 0");
          }
          if (scaleEndian > 0.0) {
            throw std::runtime_error(
                "#ospray_sg: could not parse PF PFM file '" + fileName +
                "': currently supporting only little endian formats"
                "Please report this bug at ospray.github.io, and include named "
                "file to reproduce the error.");
          }
          float scaleFactor = std::abs(scaleEndian);

          vec2i size;
          size.x        = width;
          size.y        = height;
          int channels  = numChannels;
          int depth     = sizeof(float);

          unsigned int dataSize = size.x * size.y * channels * depth;

          if (channels == 1) {
            std::vector<float> data(dataSize);
            rc = fread(data.data(), dataSize, 1, file);
            generatePFMTex<float>(data, dataSize, height, width, channels, scaleFactor);
            newTexNode->createChildData("data", data, vec2ul(width, height));
          } else if (channels == 2) {
            std::vector<vec2f> data(dataSize);
            rc = fread(data.data(), dataSize, 1, file);
            generatePFMTex<vec2f>(data, dataSize, height, width, channels, scaleFactor);
            newTexNode->createChildData("data", data, vec2ul(width, height));
          } else if (channels == 3) {
            std::vector<vec3f> data(dataSize);
            rc = fread(data.data(), dataSize, 1, file);
            generatePFMTex<vec3f>(data, dataSize, height, width, channels, scaleFactor);
            newTexNode->createChildData("data", data, vec2ul(width, height));
          } else if (channels == 4) {
            std::vector<vec4f> data(dataSize);
            rc = fread(data.data(), dataSize, 1, file);
            generatePFMTex<vec4f>(data, dataSize, height, width, channels, scaleFactor);
            newTexNode->createChildData("data", data, vec2ul(width, height));
          }

          auto texFormat = (int)(osprayTextureFormat(depth, channels, preferLinear));
          auto texFilter = (int)(nearestFilter ? OSP_TEXTURE_FILTER_NEAREST
              : OSP_TEXTURE_FILTER_BILINEAR);
          newTexNode->createChild("format", "int", texFormat);
          newTexNode->createChild("filter", "int", texFilter);
        } catch (const std::runtime_error &e) {
          std::cerr << e.what() << std::endl;
          // reset();
        }

      } else {
        int width, height, n;
        const bool hdr        = stbi_is_hdr(fileName.c_str());
        unsigned char *pixels = nullptr;
        if (hdr)
          pixels = (unsigned char *)stbi_loadf(
              fileName.c_str(), &width, &height, &n, 0);
        else
          pixels = stbi_load(fileName.c_str(), &width, &height, &n, 0);


        vec2i size;
        size.x        = width;
        size.y        = height;
        int channels  = n;
        int depth     = hdr ? 4 : 1;

        if (!pixels) {
          std::cerr << "#osp:sg: failed to load texture '" + fileName + "'"
            << std::endl;
          // reset();
        } else {
          unsigned int dataSize = size.x * size.y * channels * depth;
          if (channels == 1) {
            std::vector<unsigned char> data(dataSize);
            generateTex<unsigned char>(data, dataSize, height, width, channels, pixels, hdr);
            newTexNode->createChildData("data", data, vec2ul(width, height));
          } else if (channels == 2) {
            std::vector<vec2uc> data(dataSize);
            generateTex<vec2uc>(data, dataSize, height, width, channels, pixels, hdr);
            newTexNode->createChildData("data", data, vec2ul(width, height));
          } else if (channels == 3) {
            std::vector<vec3uc> data(dataSize);
            generateTex<vec3uc>(data, dataSize, height, width, channels, pixels, hdr);
            newTexNode->createChildData("data", data, vec2ul(width, height));
          } else if (channels == 4) {
            std::vector<vec4uc> data(dataSize);
            generateTex<vec4uc>(data, dataSize, height, width, channels, pixels, hdr);
            newTexNode->createChildData("data", data, vec2ul(width, height));
          }

          auto texFormat =
            (int)(osprayTextureFormat(depth, channels, preferLinear));
          auto texFilter = (int)(nearestFilter ? OSP_TEXTURE_FILTER_NEAREST
              : OSP_TEXTURE_FILTER_BILINEAR);
          newTexNode->createChild("format", "int", texFormat);
          newTexNode->createChild("filter", "int", texFilter);
        }
      }
#endif

      // If the load was successful, add texture node to cache and populate
      // object
      if (newTexNode->hasChild("data")) {
        newTexNode->createChild("filename", "string", baseFileName);
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
    if (newTexNode)
      for (auto &c : newTexNode->children())
        add(c.second);
  }

  OSP_REGISTER_SG_NODE_NAME(Texture2D, texture_2d);

  std::map<std::string, std::shared_ptr<Texture2D>> Texture2D::textureCache;

  }  // namespace sg
} // namespace ospray
