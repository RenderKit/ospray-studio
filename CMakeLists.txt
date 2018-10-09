## ======================================================================== ##
## Copyright 2009-2018 Intel Corporation                                    ##
##                                                                          ##
## Licensed under the Apache License, Version 2.0 (the "License");          ##
## you may not use this file except in compliance with the License.         ##
## You may obtain a copy of the License at                                  ##
##                                                                          ##
##     http://www.apache.org/licenses/LICENSE-2.0                           ##
##                                                                          ##
## Unless required by applicable law or agreed to in writing, software      ##
## distributed under the License is distributed on an "AS IS" BASIS,        ##
## WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. ##
## See the License for the specific language governing permissions and      ##
## limitations under the License.                                           ##
## ======================================================================== ##

project(ospray_studio)
cmake_minimum_required(VERSION 3.1)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

set (CMAKE_CXX_STANDARD 11)

## Find dependencies ##

find_package(OpenGL REQUIRED)
find_package(glfw3)
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE) # for gcc6
find_package(Threads REQUIRED)
find_package(ospray REQUIRED)

include(${OSPRAY_USE_FILE})

ospray_configure_tasking_system()

## Build application ##

set(OSPRAY_DEFAULT_COMPONENT apps)

include_directories(
  ${OSPRAY_INCLUDE_DIRS}
  external/
  external/imgui
)

add_subdirectory(external)
add_subdirectory(app)
