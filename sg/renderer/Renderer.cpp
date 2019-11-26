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

namespace ospray {
  namespace sg {

    Renderer::Renderer(std::string type)
    {
      auto handle = ospNewRenderer(type.c_str());
      setHandle(handle);

      createChild("spp", "int", "samples-per-pixel", 1);
      createChild("varianceThreshold",
                  "float",
                  "stop rendering when variance < threshold",
                  0.f);
      createChild("bgColor", "rgba", rgba(0.1f));
    }

    NodeType Renderer::type() const
    {
      return NodeType::RENDERER;
    }

    // Register OSPRay's debug renderers //

    OSP_REGISTER_SG_NODE_NAME(Renderer("testFrame"), renderer_testFrame);
    OSP_REGISTER_SG_NODE_NAME(Renderer("rayDir"), renderer_rayDir);

    OSP_REGISTER_SG_NODE_NAME(Renderer("raycast"), renderer_raycast);
    OSP_REGISTER_SG_NODE_NAME(Renderer("raycast_vertexColor"),
                              renderer_raycast_vertexColor);

    OSP_REGISTER_SG_NODE_NAME(Renderer("raycast_dPds"), renderer_dPds);
    OSP_REGISTER_SG_NODE_NAME(Renderer("raycast_dPdt"), renderer_dPdt);
    OSP_REGISTER_SG_NODE_NAME(Renderer("raycast_Ng"), renderer_Ng);
    OSP_REGISTER_SG_NODE_NAME(Renderer("raycast_Ns"), renderer_Ns);

    OSP_REGISTER_SG_NODE_NAME(Renderer("backfacing_Ng"),
                              renderer_backfacing_Ng);
    OSP_REGISTER_SG_NODE_NAME(Renderer("backfacing_Ns"),
                              renderer_backfacing_Ns);

    OSP_REGISTER_SG_NODE_NAME(Renderer("primID"), renderer_primID);
    OSP_REGISTER_SG_NODE_NAME(Renderer("geomID"), renderer_geomID);
    OSP_REGISTER_SG_NODE_NAME(Renderer("instID"), renderer_instID);

    OSP_REGISTER_SG_NODE_NAME(Renderer("raycast_volume"),
                              renderer_raycast_volume);

  }  // namespace sg
}  // namespace ospray
