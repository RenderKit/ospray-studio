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

#include "About.h"

#include "imgui.h"

namespace ospray {

  PanelAbout::PanelAbout() : Panel("About OSPRay Studio") {}

  void PanelAbout::buildUI()
  {
    ImGui::OpenPopup("About OSPRay Studio");

    if (ImGui::BeginPopupModal("About OSPRay Studio",
                               nullptr,
                               ImGuiWindowFlags_AlwaysAutoResize)) {
      ImGui::Text(
          R"text(
Copyright Intel Corporation 2018

This application is brought to you
by the OSPRay team.

Please check out www.sdvis.org and
www.ospray.org for more details and
future updates!

)text");

      ImGui::Separator();

      if (ImGui::Button("Close")) {
        setShown(false);
        ImGui::CloseCurrentPopup();
      }

      ImGui::EndPopup();
    }
  }

}  // namespace ospray
