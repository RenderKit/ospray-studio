## Copyright 2018-2020 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

add_executable(ospStudio
  ArcballCamera.cpp
  MainWindow.cpp
  imgui_impl_glfw.cpp
  imgui_impl_opengl2.cpp
  ospStudio.cpp
	PluginManager.cpp
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
