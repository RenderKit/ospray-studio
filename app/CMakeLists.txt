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

ospray_create_application(ospStudio
  ospStudioMain.cpp

  util/AsyncRenderEngine.cpp
  util/CameraManipulator.cpp

  widgets/imgui_impl_glfw_gl3.cpp
  widgets/imgui3D.cpp
  widgets/MainWindow.cpp

  widgets/panels/About.cpp

  widgets/sg_ui/ospray_sg_ui.cpp
  widgets/sg_ui/transferFunction.cpp

LINK
  ${OSPRAY_LIBRARIES}
  ${OSPRAY_SG_LIBRARIES}
  ospray_imgui
  ospray_tfn
  glfw
)
