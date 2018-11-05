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

#include "SGAdvanced.h"
#include "../../sg_visitors/RecomputeBounds.h"
#include "../sg_ui/ospray_sg_ui.h"
// imgui
#include "imgui.h"
// ospray_sg
#include "ospray/sg/visitor/MarkAllAsModified.h"

namespace ospray {

  PanelSGAdvanced::PanelSGAdvanced(std::shared_ptr<sg::Frame> sg)
      : Panel("Scene Graph - Advanced Tools"), scenegraph(sg)
  {
  }

  void PanelSGAdvanced::buildUI()
  {
    auto flags = g_defaultWindowFlags | ImGuiWindowFlags_NoResize |
                 ImGuiWindowFlags_AlwaysAutoResize;

    if (!ImGui::Begin("Scene Graph Advanced Tools", nullptr, flags)) {
      ImGui::End();
      return;
    }

    ImGui::NewLine();

    if (ImGui::Button("Mark All Modified"))
      scenegraph->traverse(sg::MarkAllAsModified());

    if (ImGui::Button("Verify"))
      scenegraph->verify();

    if (ImGui::Button("Compute Bounds")) {
#if 0
      auto renderer = scenegraph->child("renderer").nodeAs<sg::Renderer>();
      renderer->computeBounds();
#else
      scenegraph->traverse(sg::RecomputeBounds{});
#endif
    }

    ImGui::NewLine();
    ImGui::Separator();

    if (ImGui::Button("Close##SGAvanced"))
      setShown(false);

    ImGui::End();
  }

}  // namespace ospray
