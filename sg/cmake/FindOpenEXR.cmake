## Copyright 2009-2020 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

# Find OpenEXR headers and libraries
#
# Uses:
#   OPENEXR_ROOT
#
# If found, the following will be defined:
#   OpenEXR::IlmImf target properties
#   OPENEXR_FOUND
#   OPENEXR_INCLUDE_DIRS
#   OPENEXR_LIBRARIES

if(NOT OPENEXR_ROOT)
  set(OPENEXR_ROOT $ENV{OPENEXR_ROOT})
endif()

# detect changed OPENEXR_ROOT
if(NOT OPENEXR_ROOT STREQUAL OPENEXR_ROOT_LAST)
  unset(OPENEXR_INCLUDE_DIR CACHE)
  unset(OPENEXR_LIBRARY CACHE)
endif()

find_path(OPENEXR_ROOT include/OpenEXR/OpenEXRConfig.h
  DOC "Root of OpenEXR installation"
  HINTS ${OPENEXR_ROOT}
  PATHS
    ${PROJECT_SOURCE_DIR}/OpenEXR
    /usr/local
    /usr
    /
)

find_path(OPENEXR_INCLUDE_DIR OpenEXR/OpenEXRConfig.h
  PATHS
    ${OPENEXR_ROOT}/include NO_DEFAULT_PATH
)

set(OPENEXR_HINTS
  HINTS
    ${OPENEXR_ROOT}
  PATH_SUFFIXES
    /lib
    /lib64
    /lib-vc2015
)
set(OPENEXR_PATHS PATHS /usr/lib /usr/lib64 /lib /lib64)
find_library(OPENEXR_LIBRARY IlmImf ${OPENEXR_HINTS} ${OPENEXR_PATHS})

set(OPENEXR_ROOT_LAST ${OPENEXR_ROOT} CACHE INTERNAL "Last value of OPENEXR_ROOT to detect changes")

set(OPENEXR_ERROR_MESSAGE "OpenEXR not found in your environment. You can:
  1) install via your OS package manager, or
  2) install it somewhere on your machine and point OPENEXR_ROOT to it."
)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(OpenEXR
  ${OPENEXR_ERROR_MESSAGE}
  OPENEXR_INCLUDE_DIR OPENEXR_LIBRARY
)

if(OPENEXR_FOUND)
  set(OPENEXR_INCLUDE_DIRS ${OPENEXR_INCLUDE_DIR})
  set(OPENEXR_LIBRARIES ${OPENEXR_LIBRARY})

  add_library(OpenEXR::IlmImf UNKNOWN IMPORTED)
  set_target_properties(OpenEXR::IlmImf PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${OPENEXR_INCLUDE_DIRS}")
  set_property(TARGET OpenEXR::IlmImf APPEND PROPERTY
    IMPORTED_LOCATION "${OPENEXR_LIBRARIES}")
endif()

mark_as_advanced(OPENEXR_INCLUDE_DIR)
mark_as_advanced(OPENEXR_LIBRARY)
