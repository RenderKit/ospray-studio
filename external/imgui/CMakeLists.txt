## Copyright 2009-2020 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

set(libType STATIC)
add_library(imgui ${libType}
  imgui.cpp
  imgui_draw.cpp
  imgui_demo.cpp
  imgui_widgets.cpp
)

target_include_directories(imgui
PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>
)
