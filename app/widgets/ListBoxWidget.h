// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "sg/Node.h"
#include "sg/NodeType.h"

#include <string>
#include <vector>

using vNodePtr = std::vector<ospray::sg::NodePtr>;
using svNodePtr = std::shared_ptr<vNodePtr>;

class ListBoxWidget
{
 public:
  ListBoxWidget() {}
  bool buildUI(const char *label, int *select, vNodePtr &nodes);

 private:
  static bool getter(void *vec, int index, const char **name);
};
