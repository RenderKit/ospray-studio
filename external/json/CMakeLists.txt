## Copyright 2009 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

add_library(json INTERFACE)
target_include_directories(json
  INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>)
