## Copyright 2009-2021 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

## OpenImageIO support ##

option(ENABLE_OPENIMAGEIO "Enable OpenImageIO support" OFF)
if(ENABLE_OPENIMAGEIO)
  include(oiio)
else()
  message(STATUS "Building without OpenImageIO support. Using stb_image instead.")
endif()

## OpenVDB support ##

option(ENABLE_OPENVDB "Enable OpenVDB support" OFF)
if(ENABLE_OPENVDB)
  # Monitor <PackageName>_ROOT variables, as this mechanism is used a lot in OpenVDB.
  if(POLICY CMP0074)
    cmake_policy(SET CMP0074 NEW)
  endif()

  # Try to use the OpenVDB-provided FindOpenVDB.cmake script.
  if (${OPENVDB_ROOT})
    list(PREPEND CMAKE_MODULE_PATH ${OPENVDB_ROOT}/lib/cmake/OpenVDB)
  endif()
  find_package(OpenVDB 7.0.0 REQUIRED)
else()
  message(STATUS "Building without OpenVDB support.")
endif()

## OpenEXR support ##

option(ENABLE_EXR "Enable EXR format image writing" OFF)
if (ENABLE_EXR)
  include(openexr)
else()
  message(STATUS "Building without OpenEXR support.")
endif()

## glTF Draco mesh compression support ##

option(ENABLE_GLTF_DRACO "Enable glTF Draco mesh compression support" OFF)
if (ENABLE_GLTF_DRACO)
  include(draco)
else()
  message(STATUS "Building without glTF Draco mesh compression support.")
endif()

## Build Library ##

add_library(ospray_sg SHARED
  Data.cpp
  Mpi.cpp
  Node.cpp
  Frame.cpp
  PluginCore.cpp

  camera/Camera.cpp
  camera/Perspective.cpp
  camera/Panoramic.cpp
  camera/Orthographic.cpp

  exporter/Exporter.cpp
  exporter/PNG.cpp
  exporter/JPG.cpp
  exporter/PPM.cpp
  exporter/HDR.cpp
  exporter/EXR.cpp

  fb/FrameBuffer.cpp

  generator/Generator.cpp
  generator/ParticleVol.cpp
  generator/RandomSpheres.cpp
  generator/TutorialScene.cpp
  generator/TutorialSceneML.cpp
  generator/WaveletVolume.cpp
  generator/WaveletSlices.cpp
  generator/UnstructuredVol.cpp
  generator/TestSphere.cpp
  generator/Torus.cpp

  importer/Importer.cpp
  importer/OBJ.cpp
  importer/OBJ/tiny_obj_loader_impl.cpp
  importer/glTF.cpp
  importer/PCD.cpp
  importer/glTF/tiny_gltf_impl.cpp
  importer/glTF/gltf_types.cpp
  importer/raw.cpp
  importer/particleVolume.cpp
  importer/vdb.cpp

  renderer/Material.cpp
  renderer/MaterialReference.cpp
  renderer/MpiRaycast.cpp
  renderer/PathTracer.cpp
  renderer/Renderer.cpp
  renderer/SciVis.cpp
  renderer/AO.cpp
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
  scene/Transform.cpp
  scene/Animation.cpp

  scene/geometry/Geometry.cpp
  scene/geometry/Boxes.cpp
  scene/geometry/Isosurfaces.cpp
  scene/geometry/Planes.cpp
  scene/geometry/Spheres.cpp
  scene/geometry/Subdivision.cpp
  scene/geometry/Triangles.cpp
  scene/geometry/Curves.cpp

  scene/transfer_function/TransferFunction.cpp
  scene/transfer_function/Diverging.cpp
  scene/transfer_function/Perceptual.cpp
  scene/transfer_function/Sequential.cpp

  scene/volume/Vdb.cpp
  scene/volume/Volume.cpp
  scene/volume/ParticleVolume.cpp
  scene/volume/RawFileStructuredVolume.cpp

  scene/lights/Light.cpp
  scene/lights/LightsManager.cpp
  scene/lights/Photometric.cpp
  scene/lights/DirectionalLight.cpp
  scene/lights/AmbientLight.cpp
  scene/lights/SpotLight.cpp
  scene/lights/PointLight.cpp
  scene/lights/QuadLight.cpp
  scene/lights/HDRILight.cpp
  scene/lights/SunSky.cpp

  texture/Texture.cpp
  texture/Texture2D.cpp
  texture/TextureVolume.cpp

  ${OSPRAY_STUDIO_RESOURCE_FILE}
  )

target_compile_definitions(ospray_sg PUBLIC OSPRAY_CPP_RKCOMMON_TYPES)
target_compile_features(ospray_sg PUBLIC cxx_generic_lambdas)

target_link_libraries(ospray_sg
PUBLIC
  rkcommon::rkcommon
  ospray::ospray
  json
PRIVATE
  stb_image
  tiny_obj_loader
  tiny_gltf
)

_target_strip_and_sign(ospray_sg)

install(TARGETS ospray_sg
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    COMPONENT lib
  # on Windows put the dlls into bin
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    COMPONENT lib
)

target_include_directories(ospray_sg
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/..>
)

if (ENABLE_OPENIMAGEIO)
  target_compile_definitions(ospray_sg PRIVATE -DUSE_OPENIMAGEIO)
  target_link_libraries(ospray_sg PUBLIC OpenImageIO::OpenImageIO)
endif()

if (USE_MPI)
  target_link_libraries(ospray_sg PUBLIC MPI::MPI_CXX)
endif()

target_compile_definitions(ospray_sg
  PUBLIC
  $<$<BOOL:${ENABLE_EXR}>:STUDIO_OPENEXR>
)

if (ENABLE_EXR)
  target_compile_definitions(ospray_sg PRIVATE -DUSE_OPENEXR)
  target_link_libraries(ospray_sg
    PRIVATE
      # For OpenEXR/Imath 3.x:
      $<$<TARGET_EXISTS:OpenEXR::OpenEXR>:OpenEXR::OpenEXR>
      $<$<TARGET_EXISTS:Imath::Imath>:Imath::Imath>
      $<$<TARGET_EXISTS:Imath::Half>:Imath::Half>
      # For OpenEXR 2.4/2.5:
      $<$<TARGET_EXISTS:OpenEXR::IlmImf>:OpenEXR::IlmImf>
      $<$<TARGET_EXISTS:IlmBase::Imath>:IlmBase::Imath>
      $<$<TARGET_EXISTS:IlmBase::Half>:IlmBase::Half>
  )
  endif()

if (ENABLE_OPENVDB)
  target_compile_definitions(ospray_sg PUBLIC -DUSE_OPENVDB)
  target_link_libraries(ospray_sg PUBLIC OpenVDB::openvdb)
endif()

if (ENABLE_GLTF_DRACO)
  target_compile_definitions(ospray_sg PRIVATE -DTINYGLTF_ENABLE_DRACO)
  target_include_directories(ospray_sg PRIVATE ${draco_SOURCE_DIR}/src)
  # draco_features.h get placed just under the main build dir ???
  target_include_directories(ospray_sg  PRIVATE $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}>)
  # libdraco.a isn't found otherwise ???
  target_link_libraries(ospray_sg PRIVATE ${CMAKE_BINARY_DIR}/libdraco.a)
endif()

## Version header ##

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/version.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/version.h
  @ONLY
)

target_include_directories(ospray_sg
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)

## Build Tests ##

add_subdirectory(tests)
