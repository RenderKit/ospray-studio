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

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

## OpenImageIO support ##

option(OSPSG_OPENIMAGEIO "Enable OpenImageIO support" ON)

if(OSPSG_OPENIMAGEIO)
  find_package(OpenImageIO 2.2.3.0)
  if (NOT OpenImageIO_FOUND)
    message(STATUS "OpenImageIO not found. Using stb_image instead.")
  endif()
endif()

## Build Library ##

option(ENABLE_EXR "Enable EXR format image writing" ON)
if (ENABLE_EXR)
  find_package(OpenEXR REQUIRED)
  set(OPENEXR_TARGET OpenEXR::IlmImf)
  set(EXR_EXPORTER "exporter/EXR.cpp")
endif()

add_library(ospray_sg SHARED
  Data.cpp
  Node.cpp
  Frame.cpp

  camera/Camera.cpp
  camera/Perspective.cpp

  exporter/Exporter.cpp
  exporter/PNG.cpp
  exporter/JPG.cpp
  exporter/PPM.cpp
  exporter/HDR.cpp
  ${EXR_EXPORTER}

  fb/FrameBuffer.cpp

  generator/Generator.cpp
  generator/RandomSpheres.cpp
  generator/TutorialScene.cpp
  generator/TutorialSceneML.cpp
  generator/WaveletVolume.cpp
  generator/UnstructuredVol.cpp
  generator/TestSphere.cpp
  generator/TextureVolumeTest.cpp
  generator/Torus.cpp

  importer/Importer.cpp
  importer/OBJ.cpp
  importer/OBJ/tiny_obj_loader_impl.cpp
  importer/glTF.cpp
  importer/glTF/tiny_gltf_impl.cpp
  importer/glTF/buffer_view.cpp
  importer/glTF/gltf_types.cpp


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
  renderer/materials/Principled.cpp
  renderer/materials/ThinGlass.cpp
  renderer/materials/Velvet.cpp

  scene/World.cpp

  scene/geometry/Geometry.cpp
  scene/geometry/Boxes.cpp
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
  texture/TextureVolume.cpp
  )

target_link_libraries(ospray_sg
PUBLIC
  ospcommon::ospcommon
  ospray::ospray
  OpenImageIO::OpenImageIO
PRIVATE
  stb_image
  ${OPENEXR_TARGET}
  tiny_obj_loader
  tiny_gltf
)

install(TARGETS ospray_sg
  DESTINATION ${CMAKE_INSTALL_LIBDIR}
  COMPONENT lib
)

target_include_directories(ospray_sg
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/..>
)

if(OSPSG_OPENIMAGEIO AND OpenImageIO_FOUND)
  target_compile_definitions(ospray_sg PRIVATE -DUSE_OPENIMAGEIO)
endif()

target_compile_definitions(ospray_sg
  PUBLIC
  $<$<BOOL:${ENABLE_EXR}>:STUDIO_OPENEXR>
)

## Build Tests ##

add_subdirectory(tests)
