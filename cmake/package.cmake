## Copyright 2021-2021 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

##############################################################
# CPack specific stuff
##############################################################

## CPACK_PACKAGE_NAME defaults to PROJECT_NAME
#set(CPACK_PACKAGE_NAME "OSPRay Studio")

## CPACK_PACKAGE_FILE_NAME defaults to ${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CPACK_SYSTEM_NAME}
#set(CPACK_PACKAGE_FILE_NAME "${PROJECT_NAME}-${PROJECT_VERSION}.x86_64")

#set(CPACK_PACKAGE_ICON ${PROJECT_SOURCE_DIR}/ospray-doc/images/icon.png)
#set(CPACK_PACKAGE_RELOCATABLE TRUE)
if (APPLE AND OSPSTUDIO_SIGN_FILE)
  # on OSX we strip files during signing
  set(CPACK_STRIP_FILES FALSE)
else()
  # do not disable, stripping symbols is important for security reasons
  set(CPACK_STRIP_FILES TRUE)
endif()

## CPACK_PACKAGE_VERSION_* default to CMAKE_PROJECT_VERSION_*
# set(CPACK_PACKAGE_VERSION_MAJOR ${OSPRAY_VERSION_MAJOR})
# set(CPACK_PACKAGE_VERSION_MINOR ${OSPRAY_VERSION_MINOR})
# set(CPACK_PACKAGE_VERSION_PATCH ${OSPRAY_VERSION_PATCH})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "OSPRay Studio: An interactive visualization and ray tracing application that leverages Intel OSPRay")
set(CPACK_PACKAGE_VENDOR "Intel Corporation")
# set(CPACK_PACKAGE_CONTACT ospray@googlegroups.com)

# set(CPACK_COMPONENT_LIB_DISPLAY_NAME "Library")
# set(CPACK_COMPONENT_LIB_DESCRIPTION "The OSPRay library including documentation.")

# set(CPACK_COMPONENT_DEVEL_DISPLAY_NAME "Development")
# set(CPACK_COMPONENT_DEVEL_DESCRIPTION "Header files for C and C++ required to develop applications with OSPRay.")

# set(CPACK_COMPONENT_APPS_DISPLAY_NAME "Applications")
# set(CPACK_COMPONENT_APPS_DESCRIPTION "Example, viewer and test applications as well as tutorials demonstrating how to use OSPRay.")

# set(CPACK_COMPONENT_REDIST_DISPLAY_NAME "Redistributables")
# set(CPACK_COMPONENT_REDIST_DESCRIPTION "Dependencies of OSPRay (such as Embree, TBB, imgui) that may or may not be already installed on your system.")

# point to readme and license files
set(CPACK_RESOURCE_FILE_README ${PROJECT_SOURCE_DIR}/README.md)
set(CPACK_RESOURCE_FILE_LICENSE ${PROJECT_SOURCE_DIR}/LICENSE.txt)

# if (OSPRAY_ZIP_MODE)
#  set(CPACK_MONOLITHIC_INSTALL ON)
# else()
  # set(CPACK_COMPONENTS_ALL lib apps)
# endif()


if (WIN32) # Windows specific settings
  if (NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "Only 64bit architecture supported.")
  endif()

  #set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_FILE_NAME}.windows")

  if (OSPRAY_ZIP_MODE)
    set(CPACK_GENERATOR ZIP)
  else()
    set(CPACK_GENERATOR WIX)
    set(CPACK_WIX_ROOT_FEATURE_DESCRIPTION "${CPACK_PACKAGE_DESCRIPTION_SUMMARY}")
    set(CPACK_WIX_PROPERTY_ARPURLINFOABOUT https://github.com/ospray/ospray_studio)
  
    #set(CPACK_PACKAGE_NAME "OSPRay v${OSPRAY_VERSION}")
    #list(APPEND CPACK_COMPONENTS_ALL redist)
    #set(CPACK_PACKAGE_INSTALL_DIRECTORY "Intel\\\\OSPRay v${OSPRAY_VERSION_MAJOR}")
  
    #AFAIK, the product GUID is unique per version and can be auto-generated
    #math(EXPR _VERSION_NUMBER "10000*${CPACK_PACKAGE_VERSION_MAJOR} + 100*${CPACK_PACKAGE_VERSION_MINOR} + ${CPACK_PACKAGE_VERSION_PATCH}")
    #set(CPACK_WIX_PRODUCT_GUID "F71844B9-31BF-4B28-A7B1-5206AB2724F4")
    #AFAIK, the upgrade GUID must be specified and the same across versions to allow a new version to replace an old version
    set(CPACK_WIX_UPGRADE_GUID "F71844B9-31BF-4B28-A7B1-5206AB2724F4")
    set(CPACK_WIX_CMAKE_PACKAGE_REGISTRY TRUE)
  endif()

elseif(APPLE) # MacOSX specific settings
  configure_file(${PROJECT_SOURCE_DIR}/README.md ${PROJECT_BINARY_DIR}/ReadMe.txt COPYONLY)
  set(CPACK_RESOURCE_FILE_README ${PROJECT_BINARY_DIR}/ReadMe.txt)

  # set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_FILE_NAME}.macosx")

  if (OSPRAY_ZIP_MODE)
    set(CPACK_GENERATOR ZIP)
  else()
    set(CPACK_PACKAGING_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX})
    set(CPACK_GENERATOR productbuild)
    # set(CPACK_PACKAGE_NAME ospray-${OSPRAY_VERSION})
    set(CPACK_PACKAGE_VENDOR "intel") # creates short name com.intel.ospray.xxx in pkgutil
  endif()

else() # Linux specific settings
  set(CPACK_GENERATOR TGZ)
endif()
