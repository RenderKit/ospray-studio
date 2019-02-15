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

#include "Lights.h"
// imgui
#include "imgui.h"
// ospray_sg ui
#include "../sg_ui/ospray_sg_ui.h"

namespace ospray {

  PanelLights::PanelLights(std::shared_ptr<sg::Frame> sg)
      : Panel("Lights"), scenegraph(sg)
  {
    lights = sg->child("renderer").child("lights").shared_from_this();
  }

  void PanelLights::buildUI()
  {
    auto flags = g_defaultWindowFlags | ImGuiWindowFlags_AlwaysAutoResize;
    if (ImGui::Begin("Lights", nullptr, flags)) {
      static bool show_add_light_modal = false;
      if (ImGui::Button("Add New Light"))
        show_add_light_modal = true;

      if (show_add_light_modal)
        addLightModal(show_add_light_modal);

      guiSGTree("Lights", lights);

      ImGui::End();
    }
  }

  void PanelLights::addLightModal(bool &show_add_light_modal)
  {
    ImGui::OpenPopup("New Light");

    if (ImGui::BeginPopupModal("New Light",
                               &show_add_light_modal,
                               ImGuiWindowFlags_AlwaysAutoResize)) {
      ImGui::Text("type: ");
      ImGui::SameLine();

      static int which = 0;
      ImGui::Combo("##1", &which, "directional\0point\0quad\0ambient\0", 4);

      if (ImGui::Button("Cancel")) {
        show_add_light_modal = false;
        ImGui::CloseCurrentPopup();
      }

      ImGui::SameLine();

      if (ImGui::Button("Add")) {
        std::string type;

        switch (which) {
        case 0:
          type = "DirectionalLight";
          break;
        case 1:
          type = "PointLight";
          break;
        case 2:
          type = "QuadLight";
          break;
        case 3:
          type = "AmbientLight";
          break;
        default:
          std::cerr << "WAAAAT" << std::endl;
        }

        auto &l = lights->createChild(
            "light" + std::to_string(numCreatedLights++), type);

        l.verify();
        l.commit();

        show_add_light_modal = false;
        ImGui::CloseCurrentPopup();
      }

      ImGui::EndPopup();
    }
  }

}  // namespace ospray
