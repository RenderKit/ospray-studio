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

  struct OSPSG_INTERFACE PathTracer : public Renderer
  {
    PathTracer();
    virtual ~PathTracer() override = default;
    void preCommit() override;
  };

  OSP_REGISTER_SG_NODE_NAME(PathTracer, renderer_pathtracer);

  // PathTracer definitions ///////////////////////////////////////////////////

  PathTracer::PathTracer() : Renderer("pathtracer")
  {
    createChild("maxPathLength", "int", 100);
    createChild("roulettePathLength", "int", 10000);
    createChild("geometryLights", "bool", true);
    createChild("lightSamples", "int", -1);
    createChild("maxContribution", "float", (float)1e6);
  }

  void PathTracer::preCommit() {
    this->Renderer::preCommit();

    auto &renderer = handle();
    if(hasChild("map_backplate")) {
      auto &cppTex = child("map_backplate").valueAs<cpp::Texture>();
      renderer.setParam("map_backplate", cppTex);
    }
  }

}  // namespace ospray::sg
