## Copyright 2021 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

##
## Configure
##

if(NOT DEFINED BENCHMARK_VERSION)
  set(BENCHMARK_VERSION 1.5.5)
endif()

##
## Implementation
##

message(STATUS "Getting benchmark ${BENCHMARK_VERSION}...")

find_package(benchmark ${BENCHMARK_VERSION} QUIET)

if(NOT "${benchmark_FOUND}")

  message(STATUS "Downloading benchmark ${BENCHMARK_VERSION}...")

  include(FetchContent)

  set(_BENCHMARK_ARCHIVE_EXT "zip")

  set(BENCHMARK_ENABLE_TESTING OFF)
  set(BENCHMARK_ENABLE_GTEST_TESTS OFF)
  FetchContent_Declare(
    benchmark
    URL "https://github.com/google/benchmark/archive/refs/tags/v${BENCHMARK_VERSION}.${_BENCHMARK_ARCHIVE_EXT}"
  )

  if("${USE_BENCHMARK}")
    FetchContent_MakeAvailable(benchmark)
  else()
    ## Bypass FetchContent_MakeAvailable() shortcut to disable install
    if(NOT benchmark_POPULATED)
      FetchContent_Populate(benchmark)
      ## the subdir will still be built since targets depend on it, but it won't be installed
      add_subdirectory(${benchmark_SOURCE_DIR} ${benchmark_BINARY_DIR} EXCLUDE_FROM_ALL)
    endif()
  endif()

endif()
