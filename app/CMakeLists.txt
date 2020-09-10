## Copyright 2018-2020 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

# Build a common ospray_studio shared library
# This allows plugins to share imgui context, etc.

add_library(ospray_studio SHARED
  ArcballCamera.cpp
  imgui_impl_glfw.cpp
  imgui_impl_opengl2.cpp
  PluginManager.cpp
  widgets/TransferFunctionWidget.cpp
  widgets/FileBrowserWidget.cpp
)

if (APPLE)
  set_source_files_properties(imgui_impl_glfw_gl3.cpp GLFWSgWindow.cpp
    PROPERTIES COMPILE_FLAGS -w)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DGL_SILENCE_DEPRECATION")
endif()

target_compile_definitions(ospray_studio PUBLIC OSPRAY_CPP_RKCOMMON_TYPES)

target_link_libraries(ospray_studio
  imgui
  imguiFileDialog
  json
  rkcommon::rkcommon
  ospray::ospray
  stb_image
  glfw
  ospray_sg
)

# Now build the application against ospray_studio libary

add_executable(ospStudio
  MainWindow.cpp
  Batch.cpp
  TimeSeriesWindow.cpp
  ospStudio.cpp
)

target_link_libraries(ospStudio
  ospray_studio
)


install(TARGETS ospray_studio
  DESTINATION ${CMAKE_INSTALL_LIBDIR}
  COMPONENT lib
  # on Windows put the dlls into bin
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  COMPONENT lib
)

install(TARGETS ospStudio
  DESTINATION ${CMAKE_INSTALL_BINDIR}
  COMPONENT apps
)
