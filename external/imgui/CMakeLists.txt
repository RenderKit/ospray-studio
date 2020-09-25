## Copyright 2009-2020 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

# XXX leave imgui as SHARED library.  On macOS, statically it crashes panels
add_library(imgui SHARED
  imgui.cpp
  imgui_draw.cpp
  imgui_demo.cpp
  imgui_widgets.cpp
)

target_include_directories(imgui
PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>
)
