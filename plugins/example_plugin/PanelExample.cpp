// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "PanelExample.h"

#include "sg/visitors/GenerateImGuiWidgets.h"

#include "imgui.h"

namespace ospray {
  namespace example_plugin {

  PanelExample::PanelExample(std::shared_ptr<StudioContext> _context)
      : Panel("Example Panel", _context)
  {}

  void PanelExample::buildUI(void *ImGuiCtx)
  {
    // Need to set ImGuiContext in *this* address space
    ImGui::SetCurrentContext((ImGuiContext *)ImGuiCtx);
    ImGui::OpenPopup("Example Panel");

    if (ImGui::BeginPopupModal(
            "Example Panel", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
      ImGui::Text("%s", "Hello from the example plugin!");
      ImGui::Separator();

      ImGui::Text("%s", "Current application state");
      ImGui::Text("Frame: %p", (void *) context->frame.get());
      ImGui::Text("Arcball Camera: %p", (void *) context->arcballCamera.get());
      ImGui::Text("Current scenegraph:");
      context->frame->traverse<sg::GenerateImGuiWidgets>(
          sg::TreeState::ROOTOPEN);

      if (ImGui::Button("Close")) {
        setShown(false);
        ImGui::CloseCurrentPopup();
      }

      ImGui::EndPopup();
    }
    }

  }  // namespace example_plugin
}  // namespace ospray
