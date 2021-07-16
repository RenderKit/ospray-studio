## Copyright 2020 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

add_library(imGuIZMO STATIC
  imGuIZMOquat.cpp
)

target_link_libraries(imGuIZMO PUBLIC imgui)

target_include_directories(imGuIZMO
PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>
)

