add_library(network INTERFACE)
target_include_directories(network
  INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}>)