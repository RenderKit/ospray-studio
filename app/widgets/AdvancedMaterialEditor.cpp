// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "AdvancedMaterialEditor.h"
#include "FileBrowserWidget.h"
#include "ListBoxWidget.h"

#include "sg/NodeType.h"
#include "sg/texture/Texture2D.h"
#include "sg/visitors/GenerateImGuiWidgets.h" // TreeState

#include "rkcommon/os/FileName.h"

#include <string>
#include <vector>

#include <imgui.h>

void AdvancedMaterialEditor::buildUI(ospray::sg::NodePtr materialRegistry)
{
  static int currentMaterial = -1;
  static ListBoxWidget listWidget;
  std::vector<ospray::sg::NodePtr> materialNodes;

  for (auto &mat : materialRegistry->children())
    if (mat.second->type() == ospray::sg::NodeType::MATERIAL)
      materialNodes.push_back(mat.second);

  if (materialNodes.empty()) {
    ImGui::Text("No materials found");
    return;
  }

  // ImGui::Separator();
  if (listWidget.buildUI(
          "Materials##advanced", &currentMaterial, materialNodes)) {
  }
  if (currentMaterial != -1) {
    auto selectedMat = materialNodes.at(currentMaterial);
    auto matName = selectedMat->name();
    selectedMat->traverse<ospray::sg::GenerateImGuiWidgets>(
        ospray::sg::TreeState::ROOTOPEN);

    ImGui::Separator();

    if (ImGui::Button("Copy")) {
      copiedMat = selectedMat;
    }
    if (clipboard()) {
      if (ospray::sg::NodePtr s_copiedMat = copiedMat.lock()) {
        ImGui::SameLine();
        if (ImGui::Button("Paste")) {
          // actually create the copied material node here so we know what
          // name to give it
          auto newMat = ospray::sg::createNode(
              selectedMat->name(), s_copiedMat->subType());
          for (auto &c : s_copiedMat->children())
            if (c.second->name() != "handles")
              newMat->child(c.second->name()).setValue(c.second->value());
          materialRegistry->add(newMat);
        }
        ImGui::SameLine();
        ImGui::TextDisabled("copied: '%s'", s_copiedMat->name().c_str());
      }
    }

    ImGui::Spacing();
    ImGui::Text("Replace material");
    static int currentMatType = 0;
    const char *matTypes[] = {"principled", "carPaint", "obj", "luminous"};
    ImGui::Combo("Material types", &currentMatType, matTypes, 4);
    if (ImGui::Button("Replace##material")) {
      auto newMat = ospray::sg::createNode(matName, matTypes[currentMatType]);
      materialRegistry->add(newMat);
    }

    ImGui::Spacing();
    ImGui::Text("Add texture");
    static bool showTextureFileBrowser = false;
    static rkcommon::FileName matTexFileName("");
    static char matTexParamName[64] = "";
    ImGui::InputTextWithHint("texture##material",
        "select...",
        (char *)matTexFileName.base().c_str(),
        0);
    if (ImGui::IsItemClicked())
      showTextureFileBrowser = true;

    // Leave the fileBrowser open until file is selected
    if (showTextureFileBrowser) {
      ospray_studio::FileList fileList = {};
      if (ospray_studio::fileBrowser(fileList, "Select Texture")) {
        showTextureFileBrowser = false;
        if (!fileList.empty()) {
          matTexFileName = fileList[0];
        }
      }
    }

    ImGui::InputText(
        "paramName##material", matTexParamName, sizeof(matTexParamName));
    if (ImGui::Button("Add##materialtexture")) {
      std::string paramStr(matTexParamName);
      if (!paramStr.empty()) {
        std::shared_ptr<ospray::sg::Texture2D> sgTex =
            std::static_pointer_cast<ospray::sg::Texture2D>(
                ospray::sg::createNode(paramStr, "texture_2d"));
        sgTex->load(matTexFileName, true, false);
        auto newMat =
            ospray::sg::createNode(selectedMat->name(), selectedMat->subType());
        for (auto &c : selectedMat->children()) {
          // keep existing textures if they're not the one we're currently
          // adding/replacing
          if (c.second->name() != "handles" && c.second->name() != paramStr) {
            if (c.second->name().substr(0, 4) == "map_")
              newMat->add(c.second);
            else
              newMat->child(c.second->name()).setValue(c.second->value());
          }
        }
        newMat->add(sgTex, paramStr);
        materialRegistry->add(newMat);
      }
    }
  }
}
