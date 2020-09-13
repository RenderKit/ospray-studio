// Copyright 2018-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include <iostream>
#include <memory>

#include "ImGuiFileDialog.h"
#include "imgui.h"

#include "FileBrowserWidget.h"

namespace ospray_studio {

bool fileBrowser(
    FileList &fileList, const std::string &prompt, const FilterList &filterList)
{
  bool close = false;

  std::string filters = "";
  for (auto filter : filterList)
    filters += filter + ",";

  igfd::ImGuiFileDialog::Instance()->SetExtentionInfos(".*", ImVec4(1,1,0, 0.9));

  // Allow multiple selections
  igfd::ImGuiFileDialog::Instance()->OpenDialog(
      prompt.c_str(), prompt.c_str(), filters.c_str(), ".", 0);

  if (igfd::ImGuiFileDialog::Instance()->FileDialog(prompt.c_str())) {
    if (igfd::ImGuiFileDialog::Instance()->IsOk) {
      auto selection = igfd::ImGuiFileDialog::Instance()->GetSelection();
      // selection: first: filename, second: full path
      for (auto &s : selection)
        fileList.push_back(s.second);
    }

    igfd::ImGuiFileDialog::Instance()->CloseDialog(prompt.c_str());
    close = true;
  }

  return close;
}

} // namespace ospray_studio
