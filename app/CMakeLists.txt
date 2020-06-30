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

add_executable(ospStudio
  ArcballCamera.cpp
  MainWindow.cpp
  imgui_impl_glfw.cpp
  imgui_impl_opengl2.cpp
  ospStudio.cpp
  Batch.cpp
  TimeSeriesWindow.cpp
  widgets/TransferFunctionWidget.cpp
)

## Cereal dependency ##
add_library(cereal INTERFACE)
target_include_directories(cereal INTERFACE ./cereal/include)

if (APPLE)
  set_source_files_properties(imgui_impl_glfw_gl3.cpp GLFWSgWindow.cpp
    PROPERTIES COMPILE_FLAGS -w)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DGL_SILENCE_DEPRECATION")
endif()

target_compile_definitions(ospStudio PRIVATE OSPRAY_CPP_RKCOMMON_TYPES)

target_link_libraries(ospStudio
PRIVATE
  glfw
  imgui
  stb_image
  ospray_sg
  tiny_file_dialogs
  cereal
)

install(TARGETS ospStudio
  DESTINATION ${CMAKE_INSTALL_BINDIR}
  COMPONENT apps
)
