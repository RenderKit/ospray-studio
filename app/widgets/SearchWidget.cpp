// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "SearchWidget.h"
#include <imgui.h>

SearchWidget::SearchWidget(
    std::vector<NT> &_searchTypes, std::vector<NT> &_displayTypes, TS ts)
    : searchTypes(_searchTypes), displayTypes(_displayTypes), displayState(ts)
{}

void SearchWidget::clear()
{
  searched = false;
  searchResults.clear();
  allDisplayItems.clear(); // force generation of non-searched items
  searchTerm[0] = '\0';
  selectedResultName = "";
  numPages = 0;
  currentPage = 1;
}

void SearchWidget::search(NR root)
{
  std::string searchStr(searchTerm);

  if (searchStr.size() <= 0) {
    clear();
    return;
  }

  searched = true;
  searchResults.clear();

  // Search the entire hierarchy of passed-in root node for nodes of specific
  // searchTypes and name containing searchStr
  for (auto nt : searchTypes)
    root.traverse<ospray::sg::Search>(searchStr, nt, searchResults);
}

void SearchWidget::addSearchBarUI(NR root)
{
  if (searched && lastRoot != &root)
    clear();

  lastRoot = &root;

  ImGui::SetNextItemWidth(15.f * ImGui::GetFontSize());
  if (ImGui::InputTextWithHint("##findTransformEditor",
          "search...",
          searchTerm,
          1024,
          ImGuiInputTextFlags_CharsNoBlank | ImGuiInputTextFlags_AutoSelectAll
              | ImGuiInputTextFlags_EnterReturnsTrue)) {
    search(root);
    calculatePages();
  }
  ImGui::SameLine();
  if (ImGui::Button("find"))
    search(root);
  ImGui::SameLine();
  if (ImGui::Button("clear"))
    clear();
}

void SearchWidget::addSearchResultsUI(NR root)
{
  if (searched && lastRoot != &root)
    clear();

  lastRoot = &root;

  // If any of the search results have expired, it means one of those nodes
  // has been deleted.  Conduct search again to refresh results.
  if (searched)
    for (auto &r : searchResults)
      if (r.expired()) {
        search(root);
        break;
      }

  // Create a vector of all unsearched items matching displayTypes
  if (allDisplayItems.empty() || root.children().size() != rootSize) {
    rootSize = root.children().size();
    for (const auto child : root.children()) {
      if (isOneOf(child.second->type(), displayTypes))
        allDisplayItems.push_back(child.second);
    }
    calculatePages();
  }

  ImGui::BeginChild(
      "Results", ImVec2(0, childHeight), true, ImGuiWindowFlags_HorizontalScrollbar);
  for (int i = (currentPage - 1) * numItemsPerPage;
       i < std::min(resultsCount, currentPage * numItemsPerPage);
       i++) {
    const auto itemVector = searched ? searchResults : allDisplayItems;
    const auto node = itemVector.at(i).lock();
    auto itemTypes = searched ? searchTypes : displayTypes;
    if (node && isOneOf(node->type(), itemTypes)) {
      const bool isSelected = (selectedIndex == i);
      if (ImGui::Selectable(node->name().c_str(), isSelected)) {
        selectedIndex = i;
        selectedResultName = node->name();
        selectedResultParent =
            node->parents().front()->nodeAs<ospray::sg::Node>();
      }
      if (isSelected)
        ImGui::SetItemDefaultFocus();
    }
  }
  ImGui::EndChild();

  ImGui::Text("%u result%s", resultsCount, (resultsCount == 1 ? "" : "s"));

  // paginate results
  if (ImGui::ArrowButton("##prevPage", ImGuiDir_Left))
    currentPage = std::max(1, currentPage - 1);
  ImGui::SameLine();
  ImGui::Text("page");
  ImGui::SameLine();
  ImGui::SetNextItemWidth(20.f);
  if (ImGui::InputInt(paginateLabel.c_str(), &currentPage, 0))
    currentPage = std::min(std::max(currentPage, 1), numPages);
  ImGui::SameLine();
  if (ImGui::ArrowButton("##nextPage", ImGuiDir_Right))
    currentPage = std::min(numPages, currentPage + 1);
  ImGui::SameLine();
  ImGui::SetNextItemWidth(5.f * ImGui::GetFontSize());
  if (ImGui::BeginCombo(
          "results per page", std::to_string(numItemsPerPage).c_str())) {
    for (int i = 0; i < 4; i++) {
      const bool selected = (numItemsInd == i);
      if (ImGui::Selectable(numItemsOpt[i], selected)) {
        numItemsInd = i;
        numItemsPerPage = std::atoi(numItemsOpt[numItemsInd]);
        childHeight =
            (numItemsPerPage + 1) * ImGui::GetTextLineHeightWithSpacing();
        calculatePages();
      }
      if (selected)
        ImGui::SetItemDefaultFocus();
    }
    ImGui::EndCombo();
  }

  // Drag on numItemsPerPage box resizes results window
  if (ImGui::IsItemActive()) {
    childHeight += ImGui::GetIO().MouseDelta.y;
    childHeight =
        std::max(childHeight, 2 * ImGui::GetTextLineHeightWithSpacing());
    numItemsPerPage = childHeight / ImGui::GetTextLineHeightWithSpacing() - 1;
    calculatePages();
  }
  ospray::sg::showTooltip(
      "Alternative to preselected counts,\n"
      "drag on 'results per page' selection to resize window.\n");

  ImGui::Separator();
}

bool SearchWidget::isOneOf(NT inNodeType, std::vector<NT> &nodeTypes)
{
  for (auto nt : nodeTypes)
    if (nt == inNodeType)
      return true;
  return false;
}

void SearchWidget::addCustomAction(std::string title,
    std::function<void(ospray::sg::SearchResults &)> searchOp,
    std::function<void(ospray::sg::SearchResults &)> displayOp,
    bool sameLine)
{
  if (sameLine)
    ImGui::SameLine();
  if (ImGui::Button(title.c_str())) {
    if (searched)
      searchOp(searchResults);
    else
      displayOp(allDisplayItems);
  }
}
