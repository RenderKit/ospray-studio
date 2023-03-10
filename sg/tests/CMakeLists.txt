## Copyright 2009 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

enable_testing()

add_library(catch_main STATIC catch/catch_main.cpp)
target_link_libraries(catch_main PRIVATE ospray_sg)

if(NOT WIN32)
  add_executable(test_Node test_Node.cpp)
  target_link_libraries(test_Node PRIVATE ospray_sg catch_main)
endif()

add_executable(test_Frame test_Frame.cpp)
target_link_libraries(test_Frame PRIVATE ospray_sg)

add_executable(test_sgTutorial test_sgTutorial.cpp)
target_link_libraries(test_sgTutorial PRIVATE ospray_sg)

# Internal catch2 testing
add_test(NAME test-Node COMMAND $<TARGET_FILE:test_Node>)
add_test(NAME test-Frame COMMAND $<TARGET_FILE:test_Frame>)
add_test(NAME test-sgTutorial COMMAND $<TARGET_FILE:test_sgTutorial>)
