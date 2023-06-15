option(BUILD_PLUGIN_STORYBOARD "Storyboard plugin" OFF)

if (BUILD_PLUGIN_STORYBOARD)  
  set(pluginName "ospray_studio_plugin_storyboard")

  add_library(${pluginName} SHARED
    plugin_storyboard.cpp
    PanelStoryboard.cpp
    request/RequestManager.cpp
    request/ScreenShotContainer.cpp
  )

  target_link_libraries(${pluginName} ospray_sg ospray_ui)
  target_link_libraries(${pluginName} imgui stb_image)

  # Add external libraries for this plugin
  # add_subdirectory(external)
  target_link_libraries(${pluginName} network)

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
