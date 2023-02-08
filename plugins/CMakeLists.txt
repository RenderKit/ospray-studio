## Copyright 2009 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

# Temporary variables
set(_plugin_names)
set(_plugin_source_dirs)
set(_plugin_binary_dirs)
set(_local_plugin_names)
set(_external_plugin_source_dirs)
set(_name)
set(_source_dir)
set(_binary_dir)

# Detect plugins relative to this file.

file(GLOB _local_plugin_names RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/ *)
foreach(_name ${_local_plugin_names})
  set(_source_dir ${CMAKE_CURRENT_SOURCE_DIR}/${_name})
  set(_binary_dir ${CMAKE_CURRENT_BINARY_DIR}/${_name})

  if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${_name}/CMakeLists.txt)
    list(APPEND _plugin_names ${_name})
    list(APPEND _plugin_source_dirs ${_source_dir})
    list(APPEND _plugin_binary_dirs ${_binary_dir})
  endif()
endforeach()

# Detect plugins declared from the command line.

set(OSPRAY_STUDIO_PLUGINS "" CACHE PATH
    "OSPRay Studio will build plugins defined in these directories")

if(DEFINED OSPRAY_STUDIO_PLUGINS)
  if(NOT "${OSPRAY_STUDIO_PLUGINS}" STREQUAL "")

	set(_external_plugin_source_dirs ${OSPRAY_STUDIO_PLUGINS})
	foreach(_source_dir ${_external_plugin_source_dirs})
	  get_filename_component(_name "${_source_dir}" NAME)
	  set(_binary_dir ${CMAKE_CURRENT_BINARY_DIR}/${_name})

	  list(APPEND _plugin_names ${_name})
	  list(APPEND _plugin_source_dirs ${_source_dir})
	  list(APPEND _plugin_binary_dirs ${_binary_dir})
	endforeach()
  endif()
endif()

# CMake <3.17 doesn't have "foreach(... IN ZIP_LISTS ...)", so we have to
# manually iterate over the plugin names and directories if we want to support
# earlier versions. If we upgrade our minimum to 3.17, then the following
# foreach() loop can be simplified a lot.

list(LENGTH _plugin_names _plugin_count)
math(EXPR _plugin_last_index "${_plugin_count}-1")

foreach(_index RANGE "${_plugin_last_index}")
  list(GET _plugin_names "${_index}" _name)
  list(GET _plugin_source_dirs "${_index}" _source_dir)
  list(GET _plugin_binary_dirs "${_index}" _binary_dir)

  message(STATUS "OSPRay Studio detected plugin: ${_name}")
  add_subdirectory("${_source_dir}" "${_binary_dir}")
endforeach()

unset(_plugin_count)
unset(_plugin_last_index)
unset(_index)

# Clean up temporary variables
unset(_plugin_names)
unset(_plugin_source_dirs)
unset(_plugin_binary_dirs)
unset(_local_plugin_names)
unset(_external_plugin_source_dirs)
unset(_name)
unset(_source_dir)
unset(_binary_dir)
