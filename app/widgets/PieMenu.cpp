// Copyright 2018 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include <iostream>

#include "imgui_piemenu.h"

namespace ospray_studio {

// XXX Example mind-blowing PieMenu.... what to do with it?
// The hover/click isn't getting picked up.  The pie menu otherwise works.

void pieMenu()
{
  std::string sText;

  if (/*ImGui::IsWindowHovered() &&*/ ImGui::IsMouseClicked(1))
    ImGui::OpenPopup("PieMenu");

  if (BeginPiePopup("PieMenu", 1)) {
    if (PieMenuItem("Test1"))
      sText = "Test1";
    if (PieMenuItem("Test2")) {
      sText = "Test2";
    }
    if (PieMenuItem("Test3", false))
      sText = "Test3";
    if (BeginPieMenu("Sub")) {
      if (BeginPieMenu("Sub sub\nmenu")) {
        if (PieMenuItem("SubSub"))
          sText = "SubSub";
        if (PieMenuItem("SubSub2"))
          sText = "SubSub2";
        EndPieMenu();
      }
      if (PieMenuItem("TestSub"))
        sText = "TestSub";
      if (PieMenuItem("TestSub2"))
        sText = "TestSub2";
      EndPieMenu();
    }
    if (BeginPieMenu("Sub2")) {
      if (PieMenuItem("TestSub"))
        sText = "TestSub";
      if (BeginPieMenu("Sub sub\nmenu")) {
        if (PieMenuItem("SubSub"))
          sText = "SubSub";
        if (PieMenuItem("SubSub2"))
          sText = "SubSub2";
        EndPieMenu();
      }
      if (PieMenuItem("TestSub2"))
        sText = "TestSub2";
      EndPieMenu();
    }

    EndPiePopup();
  }
}

} // namespace ospray_studio
