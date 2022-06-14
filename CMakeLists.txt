## Copyright 2009 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.15)

project(ospray_studio VERSION 0.11.0 LANGUAGES CXX)

include(GNUInstallDirs)
include(ProcessorCount)

ProcessorCount(PROCESSOR_COUNT)
set(NUM_BUILD_JOBS ${PROCESSOR_COUNT} CACHE STRING "Number of build jobs '-j <n>'")

set(DEFAULT_BUILD_COMMAND cmake --build . --config release -j ${NUM_BUILD_JOBS})

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/install" CACHE PATH "Install location." FORCE)
endif()

set (CMAKE_CXX_STANDARD 14 CACHE STRING
     "C++ standard for preference (11, 14, 17, 20, etc.)")
set (CMAKE_CXX_STANDARD_REQUIRED ON)
set (CMAKE_CXX_EXTENSIONS OFF)
message (STATUS "This build is for C++${CMAKE_CXX_STANDARD}")

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(CONFIGURATION_TYPES "Debug;Release;RelWithDebInfo")
if (WIN32)
  if (NOT OSPRAY_DEFAULT_CMAKE_CONFIGURATION_TYPES_SET)
    set(CMAKE_CONFIGURATION_TYPES "${CONFIGURATION_TYPES}"
        CACHE STRING "List of generated configurations." FORCE)
    set(OSPRAY_DEFAULT_CMAKE_CONFIGURATION_TYPES_SET ON
        CACHE INTERNAL "Default CMake configuration types set.")
  endif()
else()
  if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the build type." FORCE)
  endif()
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS ${CONFIGURATION_TYPES})
endif()

## global options and variables
option(OSPRAY_INSTALL "Install OSPRay libraries in addition to OSPRay Studio libraries / binaries" ON)

set(OSPRAY_STUDIO_RESOURCE_FILE "${PROJECT_SOURCE_DIR}/resources/ospray_studio.rc")
if(APPLE)
  ## ensures that build-time executables contain install-time rpaths, ensuring
  ## they are identical from a code-signing POV
  set(CMAKE_BUILD_WITH_INSTALL_RPATH ON)
endif()

list(APPEND CMAKE_MODULE_PATH
  ${PROJECT_SOURCE_DIR}/cmake
)
include(functions)

## Find dependencies ##

set(OpenGL_GL_PREFERENCE "LEGACY")
find_package(OpenGL 2 REQUIRED)

include(glfw)
include(ospray)

option(USE_BENCHMARK "Build benchmarking support into ospStudio" ON)

if(USE_BENCHMARK)
  include(benchmark)
endif()

## Build application ##

set(OSPRAY_DEFAULT_COMPONENT apps)

## Optional MPI Support ##
option(USE_MPI "Enable MPI" OFF)
if (USE_MPI)
  find_package(MPI REQUIRED)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DUSE_MPI=1")
endif()

add_subdirectory(external)
add_subdirectory(sg)

## make pysg module optional

option(USE_PYSG "Enable PYSG module" ON)

if(USE_PYSG)
  include(cmake/pybind11.cmake)
  add_subdirectory(pysg)
endif()

option(BUILD_APPS "Build OSPRay Studio Apps" ON)

if (BUILD_APPS)
  add_subdirectory(app)
endif()

## Build plugins ##

option(BUILD_PLUGINS "Build plugins subtree" OFF)

if (BUILD_APPS AND BUILD_PLUGINS)
  add_subdirectory(plugins)
endif()

install(FILES
  ${PROJECT_SOURCE_DIR}/LICENSE.txt
  ${PROJECT_SOURCE_DIR}/third-party-programs.txt
  ${PROJECT_SOURCE_DIR}/third-party-programs-OSPRay.txt
  ${PROJECT_SOURCE_DIR}/CHANGELOG.md
  ${PROJECT_SOURCE_DIR}/README.md
  DESTINATION ${CMAKE_INSTALL_DOCDIR} COMPONENT apps
)

include(package)
# must be last
include(CPack)
