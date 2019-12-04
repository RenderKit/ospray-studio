## ======================================================================== ##
## Copyright 2018-2019 Intel Corporation                                    ##
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

add_executable(sgExamples
  ArcballCamera.cpp
  GLFWSgWindow.cpp
  imgui_impl_glfw.cpp
  imgui_impl_opengl2.cpp
  sgExamples.cpp
)

if (APPLE)
  set_source_files_properties(imgui_impl_glfw_gl3.cpp GLFWSgWindow.cpp
    PROPERTIES COMPILE_FLAGS -w)
endif()

target_link_libraries(sgExamples
PRIVATE
  glfw
  imgui
  ospray_sg
)

install(TARGETS sgExamples
  DESTINATION ${CMAKE_INSTALL_BINDIR}
  COMPONENT apps
)
