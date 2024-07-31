## Copyright 2020 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

if(ospray_FOUND)
    return()
endif()

## Look for any available version
message(STATUS "Looking for OSPRay...")
find_package(ospray QUIET)

if(NOT DEFINED OSPRAY_VERSION)
  set(OSPRAY_VERSION 3.2.0)
endif()

if(ospray_FOUND)
    message(STATUS "Found OSPRay")
else()

  message(STATUS "OSPRay package not found...")

    ## Download and build if not found
    if(WIN32)
        set(_ARCHIVE_EXT "windows.zip")
    elseif(APPLE)
        set(_ARCHIVE_EXT "macosx.zip")
    else()
        set(_ARCHIVE_EXT "linux.tar.gz")
    endif()

    set(_FILENAME "ospray-${OSPRAY_VERSION}.x86_64.${_ARCHIVE_EXT}")

    message(STATUS "Looking for ${_FILENAME}")

    # If a nightly build exists of requested version, use that. Otherwise pull
    # release version from github.
    if(DEFINED ENV{STORAGE_PATH})
      file(TO_CMAKE_PATH "$ENV{STORAGE_PATH}" STORAGE_PATH)
      set(_NIGHTLIES_DIR "${STORAGE_PATH}/packages/renderkit/")
      set(_LOCAL_FILE "${_NIGHTLIES_DIR}/${_FILENAME}")
      message(STATUS "Checking ${_NIGHTLIES_DIR} for nightly ${_LOCAL_FILE}")
      file(GLOB _nightly_build_files ${_NIGHTLIES_DIR}/*.${_ARCHIVE_EXT})
      foreach(_file ${_nightly_build_files})
        message(STATUS "   - ${_file}")
      endforeach()
      if(EXISTS "${_LOCAL_FILE}")
        message(STATUS "Using OSPRay nightly build")
        set(OSPRAY_URL "${_LOCAL_FILE}")
      else()
        message(STATUS "OSPRay nightly build not found")
      endif()
    endif()

    if(NOT DEFINED OSPRAY_URL)
      set(OSPRAY_URL "https://github.com/ospray/ospray/releases/download/v${OSPRAY_VERSION}/${_FILENAME}")
    endif()

    include(FetchContent)

    message(STATUS "Downloading ospray ${OSPRAY_URL}...")
    FetchContent_Declare(
        ospray
        URL "${OSPRAY_URL}"
    )
    FetchContent_MakeAvailable(ospray)
    ## This library is pre-built, so simply "find" the package
    set(ospray_ROOT "${ospray_SOURCE_DIR}")

    ## Ensure same version of TBB is found that pre-build library was compiled with.
    set(FORCE_TBB_VERSION ON)

    find_package(ospray ${OSPRAY_VERSION} REQUIRED)

    unset(${_ARCHIVE_EXT})

endif()

## The build of rkcommon depends on settings related to downloading OSPRay
include(rkcommon)
