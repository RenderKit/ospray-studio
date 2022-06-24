## Copyright 2021 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

add_library(cli11 INTERFACE)
target_include_directories(cli11
  INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>)
