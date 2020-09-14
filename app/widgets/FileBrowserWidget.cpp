// Copyright 2018-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include <iostream>
#include <memory>

#include "ImGuiFileDialog.h"
#include "imgui.h"

#include "FileBrowserWidget.h"

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

  // Allow multiple selections if requested (pass 0 as the vCountSelectionMax)
  igfd::ImGuiFileDialog::Instance()->OpenModal(prompt.c_str(),
      prompt.c_str(),
      filters.c_str(),
      defaultPath,
      allowMultipleSelection ? 0 : 1);

  if (igfd::ImGuiFileDialog::Instance()->FileDialog(prompt.c_str(),
        ImGuiWindowFlags_NoCollapse, ImVec2(512,256))) {
    if (igfd::ImGuiFileDialog::Instance()->IsOk) {
      auto selection = igfd::ImGuiFileDialog::Instance()->GetSelection();
      // selection: first: filename, second: full path
      for (auto &s : selection)
        fileList.push_back(s.second);

      // Change the default directory, so next time this opens it opens here.
      defaultPath = igfd::ImGuiFileDialog::Instance()->GetCurrentPath();
    }

    igfd::ImGuiFileDialog::Instance()->CloseDialog(prompt.c_str());
    close = true;
  }

  return close;
}

} // namespace ospray_studio
