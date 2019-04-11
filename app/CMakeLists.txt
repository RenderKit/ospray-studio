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

## Build app

add_library(ospray_studio SHARED
  imgui_impl_glfw.cpp
  imgui_impl_opengl2.cpp
  PluginManager.cpp

  app_utility/AsyncRenderEngine.cpp
  app_utility/CameraManipulator.cpp

  sg_utility/utility.cpp

  widgets/imgui3D.cpp
  widgets/MainWindow.cpp

  widgets/panels/About.cpp
  widgets/panels/Lights.cpp
  widgets/panels/NodeFinder.cpp
  widgets/panels/RenderingSettings.cpp
  widgets/panels/SGAdvanced.cpp
  widgets/panels/SGTreeView.cpp
  widgets/panels/TransferFunctionEditor.cpp

  widgets/sg_ui/ospray_sg_ui.cpp

  widgets/transfer_function/TransferFunctionWidget.cpp
)

target_link_libraries(ospray_studio
  PUBLIC
    ${OSPRAY_SG_LIBRARIES}
    ospray_tasking
    imgui
    tfn_lib
    glfw
)

add_executable(ospStudio ospStudioMain.cpp)
target_link_libraries(ospStudio PRIVATE ospray_studio)
