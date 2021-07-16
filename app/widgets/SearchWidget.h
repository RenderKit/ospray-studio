// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "sg/Node.h"
#include "sg/NodeType.h"

#include <string>
#include <vector>

using NT = ospray::sg::NodeType;
using NP = ospray::sg::NodePtr;

class SearchWidget
{
 public:
  SearchWidget(ospray::sg::Node &_root,
      std::vector<NT> &_searchTypes,
      std::vector<NT> &_displayTypes);

  void addSearchBarUI();
  void addSearchResultsUI();
  // add action item?

 private:
  void search();
  void clear();
  bool isOneOf(NT inNodeType, std::vector<NT> &nodeTypes);

  char searchTerm[1024]{""};
  bool searched{false};

  const char *numItemsOpt[4]{"10", "25", "50", "100"};
  int numItemsInd{1};
  int numItemsPerPage{25};
  int numPages{0};
  int currentPage{1};
  std::string paginateLabel{""};

  std::vector<ospray::sg::Node *> results;
  // These must be references since they contain OSPRay objects.
  // The widget will be destructed *after* ospShutdown and if it
  // contains sg nodes it will trigger warnings on exit
  std::vector<NT> &searchTypes;
  std::vector<NT> &displayTypes;
  ospray::sg::Node &root;
};

