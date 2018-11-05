// ======================================================================== //
// Copyright 2009-2018 Intel Corporation                                    //
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

#include "RenderingSettings.h"
// imgui
#include "imgui.h"
// ospray_sg ui
#include "../sg_ui/ospray_sg_ui.h"

namespace ospray {

  PanelRenderingSettings::PanelRenderingSettings(std::shared_ptr<sg::Frame> sg)
      : Panel("Renderer Settings"), scenegraph(sg)
  {
  }

  void PanelRenderingSettings::buildUI()
  {
    auto &renderer = scenegraph->child("renderer");

    //TODO: categorize/organize these controls

    //TODO: format these like the "properties editor" found in ImGui_demo.cpp

    if (ImGui::Begin(
            "Renderer Settings", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
      guiSGSingleNode("aoDistance", renderer["aoDistance"]);
      guiSGSingleNode("aoSamples", renderer["aoSamples"]);
      guiSGSingleNode("aoTransparencyEnabled", renderer["aoTransparencyEnabled"]);

      guiSGSingleNode("bgColor", renderer["bgColor"]);

      guiSGSingleNode("epsilon", renderer["epsilon"]);

      guiSGSingleNode("maxContribution", renderer["maxContribution"]);
      guiSGSingleNode("minContribution", renderer["minContribution"]);
      guiSGSingleNode("maxDepth", renderer["maxDepth"]);

      guiSGSingleNode("oneSidedLighting", renderer["oneSidedLighting"]);

      guiSGSingleNode("rendererType", renderer["rendererType"]);

      guiSGSingleNode("shadowsEnabled", renderer["shadowsEnabled"]);

      guiSGSingleNode("spp", renderer["spp"]);

      ImGui::End();
    }
  }

}  // namespace ospray
