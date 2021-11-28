## Copyright 2021 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

##
## Configure
##

if(NOT DEFINED DRACO_VERSION)
  set(DRACO_VERSION 1.4.3)
endif()

##
## Implementation
##

message(STATUS "Getting Draco ${DRACO_VERSION}...")

find_package(draco ${DRACO_VERSION} QUIET)

if(NOT "${draco_FOUND}")

  message(STATUS "Downloading draco ${DRACO_VERSION}...")

  include(FetchContent)

  set(_ARCHIVE_EXT "zip")

  # Hide advanced options from main CMake list
  mark_as_advanced(DRACO_FAST)
  mark_as_advanced(DRACO_GLTF)
  mark_as_advanced(DRACO_ANIMATION_ENCODING)
  mark_as_advanced(DRACO_BACKWARDS_COMPATIBILITY)
  mark_as_advanced(DRACO_DECODER_ATTRIBUTE_DEDUPLICATION)
  mark_as_advanced(DRACO_IE_COMPATIBLE)
  mark_as_advanced(DRACO_JS_GLUE)
  mark_as_advanced(DRACO_MAYA_PLUGIN)
  mark_as_advanced(DRACO_MESH_COMPRESSION)
  mark_as_advanced(DRACO_POINT_CLOUD_COMPRESSION)
  mark_as_advanced(DRACO_PREDICTIVE_EDGEBREAKER)
  mark_as_advanced(DRACO_STANDARD_EDGEBREAKER)
  mark_as_advanced(DRACO_TESTS)
  mark_as_advanced(DRACO_UNITY_PLUGIN)
  mark_as_advanced(DRACO_WASM)

  # draco needs to be build position independent to link with ospray_sg
  set(CMAKE_POSITION_INDEPENDENT_CODE ON)
  set(DRACO_FAST ON)
  set(DRACO_GLTF ON)
  set(DRACO_ANIMATION_ENCODING ON)
  set(DRACO_JS_GLUE OFF)

  FetchContent_Declare(
    draco
    URL "https://github.com/google/draco/archive/refs/tags/${DRACO_VERSION}.${_ARCHIVE_EXT}"
  )
  FetchContent_MakeAvailable(draco)

  unset(${_ARCHIVE_EXT})

endif()
