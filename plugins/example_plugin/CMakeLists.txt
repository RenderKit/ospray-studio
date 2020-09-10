## Copyright 2009-2020 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

option(BUILD_PLUGIN_EXAMPLE "Example plugin" OFF)

if (BUILD_PLUGIN_EXAMPLE)
  set( pluginName "ospray_studio_plugin_example")

  add_library(${pluginName} SHARED
    plugin_example.cpp
    PanelExample.cpp
  )

  target_link_libraries(${pluginName} ospray_studio)

  target_include_directories(${pluginName}
    PRIVATE ${CMAKE_SOURCE_DIR}
  )

  install(TARGETS ${pluginName}
    DESTINATION ${CMAKE_INSTALL_LIBDIR}
      COMPONENT lib
    # on Windows put the dlls into bin
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
      COMPONENT lib
  )

endif()
