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

      createChild("spp", "int", "Samples-per-pixel", 1);
      createChild("varianceThreshold",
                  "float",
                  "Stop rendering when variance < threshold",
                  0.f);
      createChild("bgColor", "rgba", "Background color", rgba(0.1f));
    }

    NodeType Renderer::type() const
    {
      return NodeType::RENDERER;
    }

    // Register OSPRay's debug renderers //

    OSP_REGISTER_SG_NODE_NAME(Renderer("testFrame"), Renderer_testFrame);
    OSP_REGISTER_SG_NODE_NAME(Renderer("rayDir"), Renderer_rayDir);

    OSP_REGISTER_SG_NODE_NAME(Renderer("raycast"), Renderer_raycast);
    OSP_REGISTER_SG_NODE_NAME(Renderer("raycast_vertexColor"),
                              Renderer_raycast_vertexColor);

    OSP_REGISTER_SG_NODE_NAME(Renderer("raycast_dPds"), Renderer_raycast_dPds);
    OSP_REGISTER_SG_NODE_NAME(Renderer("raycast_dPdt"), Renderer_raycast_dPdt);
    OSP_REGISTER_SG_NODE_NAME(Renderer("raycast_Ng"), Renderer_raycast_Ng);
    OSP_REGISTER_SG_NODE_NAME(Renderer("raycast_Ns"), Renderer_raycast_Ns);

    OSP_REGISTER_SG_NODE_NAME(Renderer("backfacing_Ng"),
                              Renderer_backfacing_Ng);
    OSP_REGISTER_SG_NODE_NAME(Renderer("backfacing_Ns"),
                              Renderer_backfacing_Ns);

    OSP_REGISTER_SG_NODE_NAME(Renderer("primID"), Renderer_primID);
    OSP_REGISTER_SG_NODE_NAME(Renderer("geomID"), Renderer_geomID);
    OSP_REGISTER_SG_NODE_NAME(Renderer("instID"), Renderer_instID);

    OSP_REGISTER_SG_NODE_NAME(Renderer("raycast_volume"),
                              Renderer_raycast_volume);

  }  // namespace sg
}  // namespace ospray
