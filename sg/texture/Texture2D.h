// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#ifdef USE_OPENIMAGEIO
#include <OpenImageIO/imageio.h>
#endif

// sg
#include "../Data.h"
#include "Texture.h"
// rkcommon
#include "rkcommon/os/FileName.h"

// std
#include <list>

namespace ospray {
namespace sg {

struct OSPSG_INTERFACE Texture2D : public Texture
{
  Texture2D();
  ~Texture2D() override;

  virtual void preCommit() override;
  virtual void postCommit() override;

  //! \brief load texture from given file or memory address.
  /*! \detailed if file does not exist, or cannot be loaded for
      some reason, return NULL. Multiple loads from the same file
      will return the *same* texture object */
  bool load(const FileName &fileName,
      const bool preferLinear = false,
      const bool nearestFilter = false,
      const int colorChannel = 4, // default to sampling all channels
      const void *memory = nullptr);

  void flipImage();
  bool isFlipped{false};

  std::string fileName;

  // UDIM public interface
  // * checkForUDIM will check a filename for the UDIM "1001" pattern, then find
  //   all other filenames in the atlas set.
  // * hasUDIM will quickly return whether a texture has been loaded as UDIM.
  // * getUDIM_scale and getUDIM_translation return the correct texture
  //   transform adjustments based on the number of tiles in the atlas.
  bool checkForUDIM(FileName filename);
  bool hasUDIM()
  {
    return udim_params.dims.x > 1;
  };

  vec2f getUDIM_scale()
  {
    const vec2i &size = udim_params.dims;
    vec2f scale{1.f};
    if (size.x > 1 || size.y > 1)
      scale = vec2f(size.x, size.y);
    return scale;
  };

  vec2f getUDIM_translation()
  {
    const vec2i &size = udim_params.dims;
    vec2f translation{0.f};
    if (size.x > 1 || size.y > 1)
      translation = vec2f(0.5f - 0.5f / size.x, 0.5f - 0.5f / size.y);
    return translation;
  };

  struct
  {
    vec2ul size{-1}; //! texture size, in pixels
    int components{0};
    int depth{0}; // bytes per texel
    bool preferLinear{false};
    bool nearestFilter{false};
    int colorChannel{4}; // sampled channel R(0), G(1), B(2), A(3), all(4)
    bool flip{true}; // flip texture data vertically when loading from file
    bool reload{false}; // force reload vs using texture cache
  } params;

 private:
  std::shared_ptr<void> texelData;
  static std::map<std::string, std::weak_ptr<Texture2D>> textureCache;

#ifdef USE_OPENIMAGEIO
  void loadTexture_OIIO(const std::string &fileName);
  template <typename T>
  void loadTexture_OIIO_readFile(std::unique_ptr<OIIO::ImageInput> &in);
#else
  void loadTexture_EXR(const std::string &fileName);
  void loadTexture_TIFF(const std::string &fileName);
  void loadTexture_PFM(const std::string &fileName);
  void loadTexture_STBi(const std::string &fileName);
  void loadTexture_PFM_readFile(FILE *file, float scaleFactor);
#endif

  // Internal helpers
  void createDataNode();
  template <typename T>
  void createDataNodeType_internal();
  template <typename T, int N>
  void createDataNodeVec_internal();
  template <typename T>
  void createDataNodeVec_internal();

  OSPTextureFormat osprayTextureFormat(int components);

  // UDIM parameters
  // udimTile defines the texture filename and the tile's start u,v coods
  using udimTile = std::pair<std::string, vec2i>;
  void loadUDIM_tiles(const FileName &_fileName);
  struct
  {
    bool loading = false;
    vec2i dims{0}; // uv tile dimensions
    std::list<udimTile> tiles;
  } udim_params;
};

} // namespace sg
} // namespace ospray
