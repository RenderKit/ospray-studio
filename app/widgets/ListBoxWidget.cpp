// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "ListBoxWidget.h"

#include <imgui.h>

bool ListBoxWidget::buildUI(const char *label, int *select, vNodePtr &nodes)
{
  if (nodes.empty())
    return false;
  return ImGui::ListBox(
      label, select, getter, static_cast<void *>(&nodes), nodes.size());
}

bool ListBoxWidget::getter(void *vec, int index, const char **name)
{
  auto nodes = static_cast<std::vector<ospray::sg::NodePtr> *>(vec);
  if (0 > index || index >= (int)nodes->size())
    return false;

  static std::string copy = "";
  copy = nodes->at(index)->name();
  *name = copy.data();
  return true;
}
