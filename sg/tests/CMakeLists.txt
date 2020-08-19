## ======================================================================== ##
## Copyright 2009-2019 Intel Corporation                                    ##
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

enable_testing()

add_library(catch_main STATIC catch/catch_main.cpp)
target_link_libraries(catch_main PRIVATE ospray::ospray)

if(NOT WIN32)
  add_executable(test_Node test_Node.cpp)
  target_link_libraries(test_Node PRIVATE ospray_sg catch_main)
endif()

add_executable(test_Frame test_Frame.cpp)
target_link_libraries(test_Frame PRIVATE ospray_sg)

add_executable(test_sgTutorial test_sgTutorial.cpp)
target_link_libraries(test_sgTutorial PRIVATE ospray_sg)
