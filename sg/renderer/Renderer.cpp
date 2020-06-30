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
#include "sg/texture/Texture2D.h"

namespace ospray::sg {

  Renderer::Renderer(std::string type)
  {
    auto handle = ospNewRenderer(type.c_str());
    setHandle(handle);

    createChild("pixelSamples", "int", "samples-per-pixel", 1);
    createChild("varianceThreshold",
                "float",
                "stop rendering when variance < threshold",
                0.f);
    createChild("backgroundColor", "rgba", rgba(0.1f));
  }

  NodeType Renderer::type() const
  {
    return NodeType::RENDERER;
  }

  void Renderer::preCommit()
  {
    auto &ren     = valueAs<cpp::Renderer>();
    const auto &c = children();
    if (c.empty())
      return;
    for (auto &child : c) {
      std::cout << child.first << std::endl;
      if (child.second->type() == NodeType::PARAMETER) {
        child.second->setOSPRayParam(child.first, ren.handle());
      }
    }
  }

  void Renderer::postCommit()
  {
    auto &ren = valueAs<cpp::Renderer>();
    ren.commit();
  }

  // Register OSPRay's debug renderers //
  struct OSPSG_INTERFACE DebugRenderer : public Renderer
  {
    DebugRenderer();
    virtual ~DebugRenderer() override = default;
  };

  DebugRenderer::DebugRenderer() : Renderer("debug")
  {
    createChild("method", "string", std::string("eyeLight"));
  }

  OSP_REGISTER_SG_NODE_NAME(DebugRenderer, renderer_debug);

}  // namespace ospray::sg
