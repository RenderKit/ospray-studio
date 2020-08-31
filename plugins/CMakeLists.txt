## Copyright 2009-2020 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

file(GLOB plugins RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/ *)
foreach(plugin ${plugins})
  if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${plugin}/CMakeLists.txt)
    add_subdirectory(${plugin})
  endif (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${plugin}/CMakeLists.txt)
endforeach(plugin ${plugins})
