## Copyright 2020 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

add_library(imguiFileDialog STATIC
  ImGuiFileDialog.cpp
)

target_link_libraries(imguiFileDialog PUBLIC imgui)

if (WIN32)
  include_directories(${CMAKE_CURRENT_LIST_DIR}/../dirent)
endif()

target_include_directories(imguiFileDialog
PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>
)
