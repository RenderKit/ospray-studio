## Copyright 2023 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

add_library(tiny_exr STATIC tinyexr_impl.cpp)

target_include_directories(tiny_exr
PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>
)
