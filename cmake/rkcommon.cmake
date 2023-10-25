## Copyright 2020 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

## rkcommon depends on tbb, so look for it first
include(tbb)

if(rkcommon_FOUND)
    return()
endif()

## Look for any available version
message(STATUS "Looking for rkcommon...")
find_package(rkcommon QUIET)

if(NOT DEFINED RKCOMMON_VERSION)
    set(RKCOMMON_VERSION 1.12.0)
endif()

if(rkcommon_FOUND)
    message(STATUS "Found rkcommon")
else()
    ## Download and build if not found
    if(NOT DEFINED RKCOMMON_GIT_REPOSITORY)
        set(RKCOMMON_GIT_REPOSITORY "https://github.com/ospray/rkcommon.git")
        set(RKCOMMON_GIT_TAG "v${RKCOMMON_VERSION}")
    endif()

    message(STATUS "Downloading rkcommon ${RKCOMMON_GIT_REPOSITORY} ${RKCOMMON_GIT_TAG}...")

    include(FetchContent)

    set(RKCOMMON_TBB_ROOT ${TBB_ROOT} CACHE INTERNAL "ensure rkcommon finds dependent tbb")
    set(BUILD_TESTING OFF CACHE INTERNAL "disable testing for rkcommon")

    ## Build rkcommon as a static lib, to prevent conflict with rkcommon binary shipped with OSPRay
    set(BUILD_SHARED_LIBS OFF)

    FetchContent_Declare(
        rkcommon
        GIT_REPOSITORY "${RKCOMMON_GIT_REPOSITORY}"
        GIT_TAG "${RKCOMMON_GIT_TAG}"
    )
    ## Bypass FetchContent_MakeAvailable() shortcut to disable install
    FetchContent_GetProperties(rkcommon)
    if(NOT rkcommon_POPULATED)
        FetchContent_Populate(rkcommon)
        ## the subdir will still be built since targets depend on it, but it won't be installed
        add_subdirectory(${rkcommon_SOURCE_DIR} ${rkcommon_BINARY_DIR} EXCLUDE_FROM_ALL)
    endif()
    add_library(rkcommon::rkcommon ALIAS rkcommon)

    if(WIN32)
        target_compile_definitions(rkcommon
            PUBLIC
                rkcommon_EXPORTS
        )
    elseif(APPLE)
    else()
        # nothing
    endif()

endif()
