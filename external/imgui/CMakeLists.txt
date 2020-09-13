## Copyright 2009-2020 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

add_library(imgui
  imgui.cpp
  imgui_draw.cpp
  imgui_demo.cpp
  imgui_widgets.cpp
#  imguifilesystem/imguifilesystem.cpp
)

target_link_libraries(imgui PUBLIC ${OPENGL_LIBRARIES} ospray_sg)

target_include_directories(imgui
PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>
)
