// ======================================================================== //
// Copyright 2009-2019 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

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
    virtual ~Texture2D() override = default;

    //! \brief load texture from given file.
    /*! \detailed if file does not exist, or cannot be loaded for
        some reason, return NULL. Multiple loads from the same file
        will return the *same* texture object */
    // TODO: verify textureCache works
    void load(const FileName &fileName,
              const bool preferLinear  = false,
              const bool nearestFilter = false);
    static void clearTextureCache();

    //! texture size, in pixels
    vec2i size{-1};
    int channels{0};
    int depth{0};
    bool preferLinear{false};
    bool nearestFilter{false};

   private:
    bool committed{false};

    static std::map<std::string, std::shared_ptr<Texture2D>> textureCache;
  };

  inline OSPTextureFormat osprayTextureFormat(int depth,
                                              int channels,
                                              bool preferLinear)
  {
    if (depth == 1) {
      if (channels == 1)
        return preferLinear ? OSP_TEXTURE_R8 : OSP_TEXTURE_L8;
      if (channels == 2)
        return preferLinear ? OSP_TEXTURE_RA8 : OSP_TEXTURE_LA8;
      if (channels == 3)
        return preferLinear ? OSP_TEXTURE_RGB8 : OSP_TEXTURE_SRGB;
      if (channels == 4)
        return preferLinear ? OSP_TEXTURE_RGBA8 : OSP_TEXTURE_SRGBA;
    } else if (depth == 4) {
      if (channels == 1)
        return OSP_TEXTURE_R32F;
      if (channels == 3)
        return OSP_TEXTURE_RGB32F;
      if (channels == 4)
        return OSP_TEXTURE_RGBA32F;
    }

    std::cerr << "#osp:sg: INVALID FORMAT " << depth << ":" << channels
              << std::endl;
    return OSP_TEXTURE_FORMAT_INVALID;
  }

  }  // namespace sg
} // namespace ospray
