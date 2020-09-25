## Copyright 2009-2020 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

# XXX make imgui a SHARED library on macOS.
# linked statically it crashes panels with an invalid ImGui context ptr
if (APPLE)
  set(libType SHARED)
else()
  set(libType STATIC)
endif()
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
