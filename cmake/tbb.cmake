## Copyright 2020 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

if(TBB_FOUND)
    return()
endif()

if(NOT DEFINED TBB_VERSION)
  set(TBB_VERSION 2021.9.0)
endif()

option(FORCE_TBB_VERSION "Force CMake to find ${TBB_VERSION} (typically for compatibility with pre-built OSPRay)" OFF)
mark_as_advanced(FORCE_TBB_VERSION)
if(FORCE_TBB_VERSION)
    message(STATUS "Looking for TBB ${TBB_VERSION}...")
    find_package(TBB ${TBB_VERSION} QUIET)
else()
    ## Look for any available version
    message(STATUS "Looking for TBB...")
    find_package(TBB QUIET)
endif()

if(TBB_FOUND)
    message(STATUS "Found TBB")
else()
    ## Download and build if not found
    if(WIN32)
        set(_ARCHIVE_EXT "win.zip")
    elseif(APPLE)
        set(_ARCHIVE_EXT "mac.tgz")
    else()
        set(_ARCHIVE_EXT "lin.tgz")
    endif()

    if(NOT DEFINED TBB_URL)
        set(TBB_URL "https://github.com/oneapi-src/oneTBB/releases/download/v${TBB_VERSION}/oneapi-tbb-${TBB_VERSION}-${_ARCHIVE_EXT}")
    endif()

    message(STATUS "Downloading TBB ${TBB_URL}...")

    include(FetchContent)

    FetchContent_Declare(
        TBB
        URL "${TBB_URL}"
    )
    FetchContent_MakeAvailable(tbb)

    ## This library is pre-built, so simply "find" the package
    set(TBB_ROOT "${tbb_SOURCE_DIR}")
    find_package(TBB ${TBB_VERSION} REQUIRED)

    unset(${_ARCHIVE_EXT})

endif()
