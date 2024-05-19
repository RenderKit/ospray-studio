option(BUILD_PLUGIN_GESTURE "Gesture plugin" OFF)

if (BUILD_PLUGIN_GESTURE)  
  set(pluginName "ospray_studio_plugin_gesture")

  add_library(${pluginName} SHARED
    plugin_gesture.cpp
    PanelGesture.cpp
    tracker/TrackingManager.cpp
  )

  target_link_libraries(${pluginName} ospray_sg)

  # Only link against imgui if needed (ie, pure file importers don't)
  target_link_libraries(${pluginName} ospray_ui)

  # There can be other plugins using async_sockets library
  if (NOT TARGET async_sockets)
    message(STATUS "Adding async_sockets library...")
    add_library(async_sockets INTERFACE)
    target_include_directories(async_sockets 
    INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/async-sockets-cpp/async-sockets/include>)
  endif()

  target_link_libraries(${pluginName} async_sockets)

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
