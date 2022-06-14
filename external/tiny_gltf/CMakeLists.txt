## Copyright 2009 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

# Prevent OSX 10.14+ OGL deprecation warnings
# This isn't set at the project level as that conflicts with ImGui setting it.
if(APPLE)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DGL_SILENCE_DEPRECATION")
endif()

add_library(tiny_gltf INTERFACE)

target_include_directories(tiny_gltf
INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/../json>
)
