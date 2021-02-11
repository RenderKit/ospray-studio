// Copyright 2009-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

// sg
#include "../Data.h"
#include "Texture.h"
// rkcommon
#include "rkcommon/os/FileName.h"

namespace ospray {
  namespace sg {

  struct OSPSG_INTERFACE Texture2D : public Texture
  {
    Texture2D();
    ~Texture2D() override;

    //! \brief load texture from given file.
    /*! \detailed if file does not exist, or cannot be loaded for
        some reason, return NULL. Multiple loads from the same file
        will return the *same* texture object */
    // TODO: verify textureCache works
    void load(const FileName &fileName,
              const bool preferLinear  = false,
              const bool nearestFilter = false);

    std::string fileName;

    //! texture size, in pixels
    vec2i size{-1};
    int components{0};
    int depth{0};
    bool preferLinear{false};
    bool nearestFilter{false};

   private:
    bool committed{false};

    static std::map<std::string, std::shared_ptr<Texture2D>> textureCache;
  };

  inline OSPTextureFormat osprayTextureFormat(int depth,
                                              int components,
                                              bool preferLinear)
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

    std::cerr << "#osp:sg: INVALID FORMAT " << depth << ":" << components
              << std::endl;
    return OSP_TEXTURE_FORMAT_INVALID;
  }

  }  // namespace sg
} // namespace ospray
