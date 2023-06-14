## Copyright 2023 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

add_library(tiny_dng STATIC tiny_dng_impl.cpp)

target_include_directories(tiny_dng
PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>
)

target_link_libraries(tiny_dng
PRIVATE
  stb_image
)
