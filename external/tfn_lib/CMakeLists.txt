## Copyright 2009-2020 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

add_library(tfn_lib tfn_lib.cpp jsoncpp.cpp)
target_link_libraries(tfn_lib PUBLIC ${OSPRAY_LIBRARIES})

option(BUILD_PARAVIEW_TFN_CVT
  "Build ParaView to OSPRay viewer transfer function converter"
  OFF
)

if (BUILD_PARAVIEW_TFN_CVT)
  add_executable(ospCvtParaViewTfcn convertParaViewTfcn.cpp jsoncpp.cpp)
  target_link_libraries(ospCvtParaViewTfcn PRIVATE tfn_lib)
endif()
