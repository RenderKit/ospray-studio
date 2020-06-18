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

#include "Renderer.h"

namespace ospray::sg {

  struct OSPSG_INTERFACE SciVis : public Renderer
  {
    SciVis();
    virtual ~SciVis() override = default;
    void preCommit() override;
    void postCommit() override;
  };

  OSP_REGISTER_SG_NODE_NAME(SciVis, renderer_scivis);

  // SciVis definitions ///////////////////////////////////////////////////////

  SciVis::SciVis() : Renderer("scivis")
  {
    createChild("aoSamples", "int", 1);
    createChild("aoRadius", "float", 1e20f);
    createChild("aoIntensity", "float", 1.f);
  }

  void SciVis::preCommit() {
//    auto &renderer = handle();
//    if(hasChild("map_backplate")) {
//      auto &cppTex = child("map_backplate").valueAs<cpp::Texture>();
//      renderer.setParam("map_backplate", cppTex);
//    } else if (hasChild("map_maxDepth")) {
//      // add texture for maxDepth here.
//    }
  }

  void SciVis::postCommit() {
    auto &renderer = handle();
    renderer.commit();
  }

}  // namespace ospray::sg
