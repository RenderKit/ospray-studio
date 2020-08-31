// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "PanelExample.h"

#include "imgui.h"

namespace ospray {
  namespace example_plugin {

    PanelExample::PanelExample() : Panel("Example Panel") {}

    void PanelExample::buildUI()
    {
      ImGui::OpenPopup("Example Panel");

      if (ImGui::BeginPopupModal(
              "Example Panel", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("%s", "Hello from the example plugin!");
        ImGui::Separator();

        if (ImGui::Button("Close")) {
          setShown(false);
          ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
      }
    }

  }  // namespace example_plugin
}  // namespace ospray
