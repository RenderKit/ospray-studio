// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "SearchWidget.h"

#include "sg/visitors/Search.h"
#include "sg/visitors/GenerateImGuiWidgets.h"

#include <imgui.h>

SearchWidget::SearchWidget(ospray::sg::Node &_root,
    std::vector<NT> &_searchTypes,
    std::vector<NT> &_displayTypes)
    : root(_root), searchTypes(_searchTypes), displayTypes(_displayTypes)
{}

void SearchWidget::clear()
{
  searched = false;
  results.clear();
  searchTerm[0] = '\0';
  numPages = 0;
}

void SearchWidget::search()
{
  std::string searchStr(searchTerm);

  if (searchStr.size() <= 0) {
    clear();
    return;
  }

  searched = true;
  results.clear();
  for (auto nt : searchTypes)
    root.traverse<ospray::sg::Search>(searchStr, nt, results);

  numPages = results.size() / numItemsPerPage;
  numPages += results.size() % numItemsPerPage == 0 ? 0 : 1;
  paginateLabel = "of " + std::to_string(numPages) + "##currentPage";
  currentPage = 1;
}

void SearchWidget::addSearchBarUI()
{
  if (ImGui::InputTextWithHint("##findTransformEditor",
          "search...",
          searchTerm,
          1024,
          ImGuiInputTextFlags_CharsNoBlank | ImGuiInputTextFlags_AutoSelectAll
              | ImGuiInputTextFlags_EnterReturnsTrue)) {
    search();
  }
  ImGui::SameLine();
  if (ImGui::Button("find")) {
    search();
  }
  ImGui::SameLine();
  if (ImGui::Button("clear")) {
    clear();
  }

  /* action items?
  if (ImGui::Button("show all")) {
    if (searched) {
      for (auto result : results)
        result->child("visible").setValue(true);
    } else {
      frame->child("world").traverse<sg::SetParamByNode>(
          NT::GEOMETRY, "visible", true);
    }
  }

  ImGui::SameLine();
  if (ImGui::Button("hide all")) {
    if (searched) {
      for (auto result : results)
        result->child("visible").setValue(false);
    } else {
      frame->child("world").traverse<sg::SetParamByNode>(
          NT::GEOMETRY, "visible", false);
    }
  }
  */
}

void SearchWidget::addSearchResultsUI()
{
  if (searched) {
    ImGui::SameLine();
    ImGui::Text(
        "%lu %s", results.size(), (results.size() == 1 ? "result" : "results"));

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
    ImGui::SameLine(0.0f, ImGui::GetFontSize() * 2.f);
    ImGui::SetNextItemWidth(5.f * ImGui::GetFontSize());
    if (ImGui::BeginCombo("results per page", numItemsOpt[numItemsInd])) {
      for (int i = 0; i < 4; i++) {
        const bool selected = (numItemsInd == i);
        if (ImGui::Selectable(numItemsOpt[i], selected)) {
          numItemsInd = i;
          numItemsPerPage = std::atoi(numItemsOpt[numItemsInd]);
          numPages = results.size() / numItemsPerPage;
          numPages += results.size() % numItemsPerPage == 0 ? 0 : 1;
          currentPage = std::min(currentPage, numPages);
          paginateLabel = "of " + std::to_string(numPages) + "##currentPage";
        }
        if (selected)
          ImGui::SetItemDefaultFocus();
      }
      ImGui::EndCombo();
    }
  }

  ImGui::BeginChild(
      "geometry", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);
  bool userUpdated = false;
  if (searched) {
    for (int i = (currentPage - 1) * numItemsPerPage;
         i < std::min((int)results.size(), currentPage * numItemsPerPage);
         i++) {
      results[i]->traverse<ospray::sg::GenerateImGuiWidgets>(
          ospray::sg::TreeState::ALLCLOSED, userUpdated);
      // Don't continue traversing
      if (userUpdated) {
        results[i]->commit();
        break;
      }
    }
  } else {
    for (auto &node : root.children()) {
      if (isOneOf(node.second->type(), displayTypes)) {
        node.second->traverse<ospray::sg::GenerateImGuiWidgets>(
            ospray::sg::TreeState::ROOTOPEN, userUpdated);
        // Don't continue traversing
        if (userUpdated) {
          break;
        }
      }
    }
  }
  ImGui::EndChild();
}

bool SearchWidget::isOneOf(NT inNodeType, std::vector<NT> &nodeTypes)
{
  for (auto nt : nodeTypes)
    if (nt == inNodeType)
      return true;
  return false;
}
