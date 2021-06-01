## Copyright 2009-2021 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

cmake_policy(SET CMP0048 NEW) # required for project version after 3.18.3
project(ospray_studio VERSION 0.7.0 LANGUAGES CXX)
cmake_minimum_required(VERSION 3.1)

if(POLICY CMP0074)
  cmake_policy(SET CMP0074 NEW) # since 3.12
endif()

include(GNUInstallDirs)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

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

## resources / copyright ##
set(OSPRAY_STUDIO_RESOURCE_FILE "${PROJECT_SOURCE_DIR}/resources/ospray_studio.rc")

## Find dependencies ##

set(OpenGL_GL_PREFERENCE "LEGACY")
find_package(OpenGL 2 REQUIRED)
find_package(glfw3 REQUIRED)
find_package(rkcommon 1.6.1 REQUIRED)
find_package(ospray 2.6.0 REQUIRED)

## Build application ##

set(OSPRAY_DEFAULT_COMPONENT apps)

add_subdirectory(external)
add_subdirectory(sg)

## make pysg module optional

option(USE_PYSG "Enable PYSG module" OFF)

if(USE_PYSG)
  find_package(pybind11 REQUIRED)
  if (NOT pybind11_FOUND)
    message(STATUS "pybind11 not found. Please install pybind11.")
  else()
  add_subdirectory(pysg)
  endif()
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
