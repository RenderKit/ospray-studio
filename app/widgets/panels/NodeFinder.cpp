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

#include "NodeFinder.h"
// ospray_sg
#include "sg/visitor/GatherNodesByName.h"
// imgui
#include "imgui.h"
// ospray_sg ui
#include "../sg_ui/ospray_sg_ui.h"
// stl
#include <array>

namespace ospray {

  PanelNodeFinder::PanelNodeFinder(std::shared_ptr<sg::Frame> sg)
    : Panel("Scene Graph - Node Finder"), scenegraph(sg)
  {
  }

  void PanelNodeFinder::buildUI()
  {
    ImGui::SetNextWindowSize(ImVec2(400,300), ImGuiCond_FirstUseEver);

    if (!ImGui::Begin("Node Finder", nullptr, g_defaultWindowFlags)) {
      ImGui::End();
      return;
    }

    ImGui::NewLine();

    std::array<char, 512> buf;
    strcpy(buf.data(), nodeNameForSearch.c_str());

    ImGui::Text("Search for node:");
    ImGui::SameLine();

    ImGui::InputText("", buf.data(), buf.size(),
                     ImGuiInputTextFlags_EnterReturnsTrue);

    std::string textBoxValue = buf.data();

    bool updateSearchResults = (nodeNameForSearch != textBoxValue);
    if (updateSearchResults) {
      nodeNameForSearch = textBoxValue;
      bool doSearch = !nodeNameForSearch.empty();
      if (doSearch) {
        sg::GatherNodesByName visitor(nodeNameForSearch);
        scenegraph->traverse(visitor);
        collectedNodesFromSearch = visitor.results();
      } else {
        collectedNodesFromSearch.clear();
      }
    }

    if (nodeNameForSearch.empty()) {
      ImGui::Text("search for: N/A");
    } else {
      const auto verifyTextLabel = std::string("search for: ")
                                   + nodeNameForSearch;
      ImGui::Text(verifyTextLabel.c_str());
    }

    if (ImGui::Button("Clear")) {
      collectedNodesFromSearch.clear();
      nodeNameForSearch.clear();
    }

    ImGui::NewLine();

    guiSearchSGNodes();

    ImGui::End();
  }

  void PanelNodeFinder::guiSearchSGNodes()
  {
    if (collectedNodesFromSearch.empty()) {
      if (!nodeNameForSearch.empty()) {
        std::string text = "No nodes found with name '";
        text += nodeNameForSearch;
        text += "'";
        ImGui::Text(text.c_str());
      }
    } else {
      for (auto &node : collectedNodesFromSearch) {
        guiSGTree("", node);
        ImGui::Separator();
      }
    }
  }

} // namespace ospray
