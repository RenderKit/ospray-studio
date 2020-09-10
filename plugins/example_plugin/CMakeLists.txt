## Copyright 2009-2020 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

option(BUILD_PLUGIN_EXAMPLE "Example plugin" OFF)

if (BUILD_PLUGIN_EXAMPLE)

  add_library(ospray_studio_plugin_example SHARED
    plugin_example.cpp
    PanelExample.cpp
  )

  target_link_libraries(ospray_studio_plugin_example ospray_studio)

  target_include_directories(ospray_studio_plugin_example
    PRIVATE ${CMAKE_SOURCE_DIR}
  )

endif()
