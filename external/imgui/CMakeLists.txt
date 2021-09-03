## Copyright 2009-2020 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

set(libType STATIC)
add_library(imgui ${libType}
  imgui.cpp
  imgui_draw.cpp
  imgui_widgets.cpp
  imgui_tables.cpp
  imgui_piemenu.cpp
)

add_custom_command(TARGET imgui POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
  ${CMAKE_CURRENT_SOURCE_DIR}/Roboto-Regular.ttf
  ${CMAKE_BINARY_DIR}
)

install(FILES Roboto-Regular.ttf
  DESTINATION ${CMAKE_INSTALL_BINDIR}
  COMPONENT apps
)

target_include_directories(imgui
PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>
)
