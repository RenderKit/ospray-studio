## ======================================================================== ##
## Copyright 2009-2020 Intel Corporation                                    ##
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

## Build Library ##

add_library(ospray_sg SHARED
  Data.cpp
  Node.cpp
  Frame.cpp

  camera/Camera.cpp
  camera/Perspective.cpp

  fb/FrameBuffer.cpp

  generator/Generator.cpp
  generator/RandomSpheres.cpp
  generator/TutorialScene.cpp
  generator/WaveletVolume.cpp
  generator/UnstructuredVol.cpp
  generator/TestSphere.cpp

  importer/Importer.cpp
  importer/OBJ.cpp
  importer/OBJ/tiny_obj_loader_impl.cpp

  renderer/Material.cpp
  renderer/MaterialReference.cpp
  renderer/PathTracer.cpp
  renderer/Renderer.cpp
  renderer/SciVis.cpp
  renderer/MaterialRegistry.cpp

  renderer/materials/OBJ.cpp
  renderer/materials/Alloy.cpp
  renderer/materials/Glass.cpp
  renderer/materials/CarPaint.cpp
  renderer/materials/Luminous.cpp
  renderer/materials/Metal.cpp
  renderer/materials/MetallicPaint.cpp
  renderer/materials/Mix.cpp
  renderer/materials/Plastic.cpp
  renderer/materials/ThinGlass.cpp

  scene/World.cpp

  scene/geometry/Geometry.cpp
  scene/geometry/Spheres.cpp
  scene/geometry/Triangles.cpp

  scene/transfer_function/Cloud.cpp
  scene/transfer_function/Jet.cpp
  scene/transfer_function/TransferFunction.cpp

  scene/volume/Structured.cpp
  scene/volume/Unstructured.cpp
  scene/volume/Volume.cpp

  scene/lights/Light.cpp
  scene/lights/DirectionalLight.cpp
  scene/lights/AmbientLight.cpp
  scene/lights/SpotLight.cpp
  scene/lights/PointLight.cpp
  scene/lights/QuadLight.cpp
  scene/lights/HDRILight.cpp

  texture/Texture.cpp
  texture/Texture2D.cpp
  )

target_link_libraries(ospray_sg
PUBLIC
  ospcommon::ospcommon
  ospray::ospray
PRIVATE
  tiny_obj_loader
)

install(TARGETS ospray_sg
  DESTINATION ${CMAKE_INSTALL_LIBDIR}
  COMPONENT lib
)

target_include_directories(ospray_sg
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/..>
)

## Build Tests ##

add_subdirectory(tests)
