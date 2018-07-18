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

#include "SGTreeView.h"

#include "imgui.h"

#include "../sg_ui/ospray_sg_ui.h"

namespace ospray {

  PanelSGTreeView::PanelSGTreeView(std::shared_ptr<sg::Frame> sg)
  {
    name = "Scene Graph Tree View";

    scenegraph = sg;
  }

  void PanelSGTreeView::buildUI()
  {
    ImGui::SetNextWindowSize(ImVec2(500,400), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("SceneGraph", &show, g_defaultWindowFlags))
      guiSGTree("root", scenegraph);

    ImGui::End();
  }

} // namespace ospray
