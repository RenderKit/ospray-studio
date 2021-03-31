## Copyright 2021 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

## manually add python flags to avoid compile time errors 
set(CMAKE_CXX_FLAGS "-I/usr/include/python3.8 -lpython3.8")

pybind11_add_module(pysg pysg.cpp ../app/ArcballCamera.cpp)

target_include_directories(pysg
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/..>
)

target_link_libraries(pysg
PUBLIC
  rkcommon::rkcommon
  ospray::ospray
  json
  ospray_sg
)

# ## Version header ##

configure_file(
  ../sg/version.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/version.h
  @ONLY
)

target_include_directories(pysg
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)
