// Copyright 2018-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include <iostream>
#include <memory>

#include "ImGuiFileDialog.h"
#include "imgui.h"

#include "FileBrowserWidget.h"

using namespace IGFD;
namespace ospray_studio {

bool fileBrowser(FileList &fileList,
    const std::string &prompt,
    const bool allowMultipleSelection,
    const FilterList &filterList)
{
  static std::string defaultPath = ".";
  bool close = false;

  std::string filters = "";
  for (auto filter : filterList)
    filters += filter + ",";

  // Set min and max dialog window size based on main window size
  ImVec2 maxSize = ImGui::GetIO().DisplaySize;
  ImVec2 minSize(maxSize.x * 0.5, maxSize.y * 0.5);

  // Allow multiple selections if requested (pass 0 as the vCountSelectionMax)
  ImGuiFileDialog::Instance()->OpenModal(prompt.c_str(),
      prompt.c_str(),
      filters.c_str(),
      defaultPath,
      "",
      allowMultipleSelection ? 0 : 1);

  if (ImGuiFileDialog::Instance()->Display(prompt.c_str(),
        ImGuiWindowFlags_NoCollapse, minSize, maxSize)) {
    if (ImGuiFileDialog::Instance()->IsOk()) {
      auto selection = ImGuiFileDialog::Instance()->GetSelection();
      // selection: first: filename, second: full path
      for (auto &s : selection)
        fileList.push_back(s.second);

      // Change the default directory, so next time this opens it opens here.
      defaultPath = ImGuiFileDialog::Instance()->GetCurrentPath();
    }

    ImGuiFileDialog::Instance()->Close();
    close = true;
  }

  return close;
}

} // namespace ospray_studio
