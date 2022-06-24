## Copyright 2009 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

# Find OpenVDB headers and libraries
# Minimal when system version isn't provided
#
# Uses:
#   OPENVDB_ROOT
#
# If found, the following will be defined:
#   OpenVDB::openvdb target properties
#   OPENVDB_FOUND
#   OPENVDB_INCLUDE_DIRS
#   OPENVDB_LIBRARIES

if(NOT OPENVDB_ROOT)
  set(OPENVDB_ROOT $ENV{OPENVDB_ROOT})
endif()

# detect changed OPENVDB_ROOT
if(NOT OPENVDB_ROOT STREQUAL OPENVDB_ROOT_LAST)
  unset(OPENVDB_INCLUDE_DIR CACHE)
  unset(OPENVDB_LIBRARY CACHE)
endif()

find_path(OPENVDB_ROOT include/openvdb/openvdb.h
  DOC "Root of OpenVDB installation"
  HINTS ${OPENVDB_ROOT}
  PATHS
    ${PROJECT_SOURCE_DIR}/openvdb
    /usr/local
    /usr
    /
)

find_path(OPENVDB_INCLUDE_DIR openvdb/openvdb.h
  PATHS
  ${OPENVDB_ROOT}/include NO_DEFAULT_PATH
)

set(OPENVDB_HINTS
  HINTS
  ${OPENVDB_ROOT}
  PATH_SUFFIXES
    /lib
    /lib64
    /lib-vc2015
)
set(OPENVDB_PATHS PATHS /usr/lib /usr/lib64 /lib /lib64)
find_library(OPENVDB_LIBRARY openvdb ${OPENVDB_HINTS} ${OPENVDB_PATHS})

set(OPENVDB_ROOT_LAST ${OPENVDB_ROOT} CACHE INTERNAL "Last value of OPENVDB_ROOT to detect changes")

set(OPENVDB_ERROR_MESSAGE "OpenVDB not found in your environment. You can:
    1) install via your OS package manager, or
    2) install it somewhere on your machine and point OPENVDB_ROOT to it."
)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(OpenVDB
  ${OPENVDB_ERROR_MESSAGE}
  OPENVDB_INCLUDE_DIR OPENVDB_LIBRARY
)

if(OPENVDB_FOUND)

  # Extract the version we found in our root.
  file(READ ${OPENVDB_INCLUDE_DIR}/openvdb/version.h VERSION_HEADER_CONTENT)
  string(REGEX MATCH "#define OPENVDB_LIBRARY_MAJOR_VERSION_NUMBER ([0-9]+)" DUMMY "${VERSION_HEADER_CONTENT}")
  set(OPENVDB_VERSION_MAJOR ${CMAKE_MATCH_1})
  string(REGEX MATCH "#define OPENVDB_LIBRARY_MINOR_VERSION_NUMBER ([0-9]+)" DUMMY "${VERSION_HEADER_CONTENT}")
  set(OPENVDB_VERSION_MINOR ${CMAKE_MATCH_1})
  string(REGEX MATCH "#define OPENVDB_LIBRARY_PATCH_VERSION_NUMBER ([0-9]+)" DUMMY "${VERSION_HEADER_CONTENT}")
  set(OPENVDB_VERSION_PATCH ${CMAKE_MATCH_1})
  set(OPENVDB_VERSION "${OPENVDB_VERSION_MAJOR}.${OPENVDB_VERSION_MINOR}.${OPENVDB_PATCH_MINOR}")
  set(OPENVDB_VERSION_STRING "${OPENVDB_VERSION}")

  message(STATUS "Found OpenVDB version ${OPENVDB_VERSION}")

  # If the user provided information about required versions, check them!
  if (OPENVDB_FIND_VERSION)
    if (${OPENVDB_FIND_VERSION_EXACT} AND NOT
        OPENVDB_VERSION VERSION_EQUAL ${OPENVDB_FIND_VERSION})
      message(ERROR "Requested exact OpenVDB version ${OPENVDB_FIND_VERSION},"
        " but found ${OPENVDB_VERSION}")
    elseif(OPENVDB_VERSION VERSION_LESS ${OPENVDB_FIND_VERSION})
      message(ERROR "Requested minimum OpenVDB version ${OPENVDB_FIND_VERSION},"
        " but found ${OPENVDB_VERSION}")
    endif()
  endif()

  set(OPENVDB_INCLUDE_DIRS ${OPENVDB_INCLUDE_DIR})
  set(OPENVDB_LIBRARIES ${OPENVDB_LIBRARY})

  add_library(OpenVDB::openvdb UNKNOWN IMPORTED)
  set_target_properties(OpenVDB::openvdb PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${OPENVDB_INCLUDE_DIRS}")
  set_property(TARGET OpenVDB::openvdb APPEND PROPERTY
    IMPORTED_LOCATION "${OPENVDB_LIBRARIES}")
endif()

mark_as_advanced(OPENVDB_INCLUDE_DIR)
mark_as_advanced(OPENVDB_LIBRARY)
