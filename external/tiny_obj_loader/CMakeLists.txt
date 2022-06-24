## Copyright 2009 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

add_library(tiny_obj_loader INTERFACE)

target_include_directories(tiny_obj_loader
INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>
)
