// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "AdvancedMaterialEditor.h"
#include "FileBrowserWidget.h"
#include "GenerateImGuiWidgets.h" // TreeState
#include "ListBoxWidget.h"

#include "sg/NodeType.h"
#include "sg/texture/Texture2D.h"

#include "rkcommon/math/AffineSpace.h"
#include "rkcommon/os/FileName.h"

#include <string>
#include <vector>

#include <imgui.h>

using namespace ospray::sg;
using namespace rkcommon::math;

void AdvancedMaterialEditor::buildUI(
    NodePtr materialRegistry, NodePtr &selectedMat)
{
  auto matName = selectedMat->name();
  updateTextureNames(selectedMat);

  if (ImGui::Button("Copy")) {
    copiedMat = selectedMat;
  }
  if (clipboard()) {
    if (NodePtr s_copiedMat = copiedMat.lock()) {
      ImGui::SameLine();
      if (ImGui::Button("Paste")) {
        // actually create the copied material node here so we know what
        // name to give it
        auto newMat = copyMaterial(s_copiedMat, selectedMat->name());
        materialRegistry->add(newMat);
      }
      ImGui::SameLine();
      ImGui::TextDisabled("copied: '%s'", s_copiedMat->name().c_str());
    }
  }

  ImGui::Spacing();
  ImGui::Text("Replace material");
  static int currentMatType = 0;
  const char *matTypes[] = {"alloy",
      "carPaint",
      "glass",
      "luminous",
      "metal",
      "metallicPaint",
      // "mix",   Mix needs pointers to two existing materials and a blend factor
      "obj",
      "plastic",
      "principled",
      "thinGlass",
      "velvet"};
  constexpr int numMatTypes = sizeof(matTypes) >> 3;
  ImGui::Combo("Material types", &currentMatType, matTypes, numMatTypes);
  if (ImGui::Button("Replace##material")) {
    auto newMat = createNode(matName, matTypes[currentMatType]);
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
      // color textures are typically sRGB gamma encoded, others prefer linear
      // texture format may override this preference
      bool preferLinear = paramStr.find("Color") == paramStr.npos
          && paramStr.find("kd") == paramStr.npos
          && paramStr.find("ks") == paramStr.npos;

      auto sgTex = createNodeAs<Texture2D>(paramStr, "texture_2d");
      // Set parameters affecting texture load and usage
      sgTex->samplerParams.preferLinear = preferLinear;
      // If load fails, remove the texture node
      if (!sgTex->load(matTexFileName))
        sgTex = nullptr;
      else {
        auto newMat = copyMaterial(selectedMat, "", paramStr);
        newMat->add(sgTex, paramStr);
        newMat->createChild(paramStr + ".transform", "linear2f") =
            linear2f(one);
        newMat->createChild(paramStr + ".translation", "vec2f") = vec2f(0.f);
        newMat->child(paramStr + ".translation").setMinMax(-1.f, 1.f);
        materialRegistry->add(newMat);
      }
    }
  }

  ImGui::Spacing();
  ImGui::Text("Remove texture");
  static int removeIndex = 0;
  static std::string texToRemove = (currentMaterialTextureNames.size() > 0)
      ? currentMaterialTextureNames[removeIndex]
      : "";
  if (ImGui::BeginCombo("Texture##materialtexture", texToRemove.c_str(), 0)) {
    for (int i = 0; i < currentMaterialTextureNames.size(); i++) {
      const bool isSelected = (removeIndex == i);
      if (ImGui::Selectable(
              currentMaterialTextureNames[i].c_str(), isSelected)) {
        removeIndex = i;
        texToRemove = currentMaterialTextureNames[removeIndex];
      }
      if (isSelected)
        ImGui::SetItemDefaultFocus();
    }
    ImGui::EndCombo();
  }
  if (ImGui::Button("Remove##materialtexturebutton")) {
    auto newMat = copyMaterial(selectedMat, "", texToRemove);
    materialRegistry->add(newMat);
    texToRemove = "";
  }
}
