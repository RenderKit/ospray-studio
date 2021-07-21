// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "sg/Node.h"
#include "sg/NodeType.h"

#include <string>
#include <vector>

using NR = ospray::sg::Node &;
using NT = ospray::sg::NodeType;
using NP = ospray::sg::NodePtr;

/* SearchWidget
 * This class implements a search box and results list UI. The results list
 * works in two ways.
 *
 * When there is no search term, the list is in "display" mode (i.e. `searched`
 * is false), and it lists the appropriate nodes under `root` based on
 * `displayTypes`.
 *
 * If a search has been performed (`search` is true), then the list is in
 * "search" mode and lists the contents of `results`. The search itself is
 * limited to node types in `searchTypes`.
 *
 * Custom actions allow for actions on either displayed or searched nodes (e.g.
 * toggle visibility).
 */

class SearchWidget
{
 public:
  SearchWidget(std::vector<NT> &_searchTypes, std::vector<NT> &_displayTypes);

  void addSearchBarUI(NR root);
  void addSearchResultsUI(NR root);

  void addCustomAction(std::string title,
      std::function<void(std::vector<ospray::sg::NodePtr> &)> searchOp,
      std::function<void()> displayOp,
      bool sameLine=false);

 private:
  void search(NR root);
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

  std::vector<ospray::sg::NodePtr> results;
  // These must be references since they contain OSPRay objects.
  // The widget will be destructed *after* ospShutdown and if it
  // contains sg nodes it will trigger warnings on exit
  std::vector<NT> &searchTypes;
  std::vector<NT> &displayTypes;
  ospray::sg::Node *lastRoot;
};
