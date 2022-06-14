## Copyright 2020 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

##
## Configure
##

if(NOT DEFINED PYBIND11_VERSION)
  set(PYBIND11_VERSION 2.6.2)
endif()

##
## Implementation
##

message(STATUS "Getting pybind11 ${PYBIND11_VERSION}...")

find_package(Python3 COMPONENTS Interpreter Development REQUIRED)
find_package(pybind11 ${PYBIND11_VERSION} QUIET)

if(NOT "${pybind11_FOUND}")

  message(STATUS "Downloading pybind11 ${PYBIND11_VERSION}...")

  include(FetchContent)

  set(_PYBIND11_ARCHIVE_EXT "zip")

  FetchContent_Declare(
    pybind11
    URL "https://github.com/pybind/pybind11/archive/refs/tags/v${PYBIND11_VERSION}.${_PYBIND11_ARCHIVE_EXT}"
  )
  ## Bypass FetchContent_MakeAvailable() shortcut to disable install
  FetchContent_GetProperties(pybind11)
  if(NOT pybind11_POPULATED)
    FetchContent_Populate(pybind11)
    ## the subdir will still be built since targets depend on it, but it won't be installed
    add_subdirectory(${pybind11_SOURCE_DIR} ${pybind11_BINARY_DIR} EXCLUDE_FROM_ALL)
  endif()

endif()
