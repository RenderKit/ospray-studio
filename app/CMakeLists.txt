## Copyright 2018-2020 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

if (APPLE)
  set_source_files_properties(imgui_impl_glfw_gl3.cpp GLFWSgWindow.cpp
    PROPERTIES COMPILE_FLAGS -w)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DGL_SILENCE_DEPRECATION")
endif()

add_executable(ospStudio
  ospStudio.cpp
  PluginManager.cpp

  MainWindow.cpp
  Batch.cpp
  TimeSeriesWindow.cpp
  AnimationManager.cpp

  ${OSPRAY_STUDIO_RESOURCE_FILE}
)

target_compile_definitions(ospStudio PRIVATE OSPRAY_CPP_RKCOMMON_TYPES)

add_library(ospray_ui STATIC
  ArcballCamera.cpp
  imgui_impl_glfw.cpp
  imgui_impl_opengl2.cpp
  widgets/TransferFunctionWidget.cpp
  widgets/FileBrowserWidget.cpp
  widgets/AnimationWidget.cpp
)

target_link_libraries(ospray_ui
  json
  glfw
  imgui
  imguiFileDialog
  ${OPENGL_LIBRARIES}
  ospray_sg
)

target_link_libraries(ospStudio
  json
  ospray_ui # Not all run modes should link against UI (ie batch).
  ospray_sg
)

install(TARGETS ospStudio
  DESTINATION ${CMAKE_INSTALL_BINDIR}
  COMPONENT apps
)
