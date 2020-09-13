## Copyright 2020 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

add_library(imguiFileDialog
  ImGuiFileDialog.cpp
)

target_link_libraries(imguiFileDialog PUBLIC imgui)

target_include_directories(imguiFileDialog
PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>
)
