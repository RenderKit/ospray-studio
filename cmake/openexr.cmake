## Copyright 2020 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

if(OpenEXR_FOUND)
    return()
endif()

# Find package installed OpenEXR (v2.x or v3.x)
# Imath indicates OpenEXR 3.x
message(STATUS "Looking for package Imath.")
find_package(Imath CONFIG)
if (NOT TARGET Imath::Imath)
  # Couldn't find Imath::Imath, find IlmBase (OpenEXR 2.x)
  set(_ASSUME_VERSION 2)
  message(STATUS "Could not find target Imath::Imath, likely OpenEXR 2.x.")
  message(STATUS "Looking for package IlmBase")
  find_package(IlmBase CONFIG)
  if (NOT TARGET IlmBase_FOUND)
    message(STATUS "Could not find IlmBase")
  endif ()
else ()
  message(STATUS "Found target Imath::Imath, likely OpenEXR 3.x.")
  set(_ASSUME_VERSION 3)
endif ()

message(STATUS "Looking for package OpenEXR, required version ${_ASSUME_VERSION}")

find_package(OpenEXR ${_ASSUME_VERSION} CONFIG REQUIRED)

if(OpenEXR_FOUND)
  message(STATUS "Found OpenEXR ${_ASSUME_VERSION}.x")
else()
  message(STATUS "Could not find OpenEXR ${_ASSUME_VERSION}.x")
endif ()

unset(_ASSUME_VERSION)
