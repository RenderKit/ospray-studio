## Copyright 2021 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

## manually add python flags to avoid compile time errors 
# set(CMAKE_CXX_FLAGS "-I/usr/include/python3.8 -lpython3.8")

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
  cli11
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

if(APPLE)
  set_target_properties(pysg PROPERTIES
    SKIP_INSTALL_RPATH OFF
    INSTALL_RPATH "@loader_path;${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}"
  )
elseif(UNIX)
  set_target_properties(pysg PROPERTIES
    SKIP_INSTALL_RPATH OFF
    INSTALL_RPATH "\$ORIGIN"
    LINK_OPTIONS "LINKER:-disable-new-dtags"
  )
endif()

_target_strip_and_sign(pysg)

if(WIN32)
  install(TARGETS pysg
    DESTINATION ${CMAKE_INSTALL_BINDIR}
      COMPONENT lib
  )
else()
  install(TARGETS pysg
    DESTINATION ${CMAKE_INSTALL_LIBDIR}
      COMPONENT lib
  )
endif()

