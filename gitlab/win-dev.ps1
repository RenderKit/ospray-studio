#!/usr/bin/env pwsh
## Copyright 2009-2021 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

# XXX Expand to handle either source or release binaries
# Are github (release and devel) branches enough for OSPRay/RKCommon?

# Stop script on first failure
$ErrorActionPreference = "Stop"

# Versions if not set by CI (standalone run)
if (! $OSPRAY_VER )   { set OSPRAY_VER "2.6.0"   }
if (! $RKCOMMON_VER ) { set RKCOMMON_VER "1.6.1" }
if (! $GLFW_VER )     { set GLFW_VER "3.3.2"     }

# Pull OSPRay and RKCommon releases from OSPRay github
set GITHUB_HOME "https://github.com/ospray"

# OSPRay
#rel ex: https://github.com/ospray/ospray/releases/download/v2.3.0/ospray-2.3.0.x86_64.windows.zip
#src ex: https://github.com/ospray/ospray/archive/devel.zip
set OSPRAY_DIR "ospray-${OSPRAY_VER}.x86_64.windows"
set OSPRAY_ZIP "${OSPRAY_DIR}.zip"
set OSPRAY_LINK "${GITHUB_HOME}/ospray/releases/download/v${OSPRAY_VER}/${OSPRAY_ZIP}"

# RKCommon
#ex: https://github.com/ospray/rkcommon/archive/v1.6.0.zip
set RKCOMMON_DIR "rkcommon-${RKCOMMON_VER}"
set RKCOMMON_ZIP "v${RKCOMMON_VER}.zip"
set RKCOMMON_LINK "${GITHUB_HOME}/rkcommon/archive/${RKCOMMON_ZIP}"

# GLFW
#ex: https://github.com/glfw/glfw/releases/download/3.3.2/glfw-3.3.2.zip
set GLFW_DIR "glfw-${GLFW_VER}"
set GLFW_ZIP "${GLFW_DIR}.zip"
set GLFW_LINK "https://github.com/glfw/glfw/releases/download/${GLFW_VER}/${GLFW_ZIP}"

set DEPENDENCIES "dependencies-${OSPRAY_DIR}-${RKCOMMON_DIR}-${GLFW_DIR}"
set DEPENDENCIES_DIR "./${DEPENDENCIES}"
set DEPENDENCIES_ZIP "${DEPENDENCIES}.zip"

# Build all Windows things into build-win
pushd .
md -Force build-win
cd build-win
set BUILD_DIR ${PWD}

cmake --version

# # If cached pre-built dependencies exist, use them
# # XXX This should look to a build cache on NAS or something
# if (-not (${DEPENDENCIES_DIR} | Test-Path) -and ${DEPENDENCIES_ZIP} | Test-Path) {
#   Expand-Archive -Path ${DEPENDENCIES_ZIP} -DestinationPath .
# }

# # Otherwise, build all dependencies
# # This downloads an OSPRay tagged release from GitHub,
# # and builds RKCommon and glfw from GitHub tagged source
# if (-not (${DEPENDENCIES_DIR} | Test-Path)) {

#   if (-not (${OSPRAY_DIR} | Test-Path)) {
#     cd ${BUILD_DIR}

#     echo "Fetching OSPRay release ${OSPRAY_VER}"
#     wget ${OSPRAY_LINK} -outfile ${OSPRAY_ZIP}

#     Expand-Archive -Path ${OSPRAY_ZIP} -DestinationPath .
#     rm ${OSPRAY_ZIP}

#   } else {
#     echo "OSPRay release exists"
#   }

#   if (-not (${RKCOMMON_DIR} | Test-Path)) {
#     cd ${BUILD_DIR}

#     echo "Fetching RKCommon release ${RKCOMMON_VER}"
#     wget ${RKCOMMON_LINK} -outfile ${RKCOMMON_ZIP}

#     Expand-Archive -Path ${RKCOMMON_ZIP} -DestinationPath .
#     rm ${RKCOMMON_ZIP}

#     echo "Building RKCommon"
#     cd ${RKCOMMON_DIR}
#     md install
#     md build
#     cd build
#     # XXX How difficult to install TBB to use TBB rather than INTERNAL tasking system?
#     cmake -L `
#       -D CMAKE_INSTALL_PREFIX="../install" `
#       -D BUILD_TESTING=OFF `
#       -D RKCOMMON_TASKING_SYSTEM=INTERNAL `
#       ..
#     cmake --build . --parallel -j --config Release --target install

#   } else {
#     echo "RKCommon release exists"
#   }

#   if (-not (${GLFW_DIR} | Test-Path)) {
#     cd ${BUILD_DIR}

#     echo "Fetching glfw release ${GLFW_VER}"
#     wget ${GLFW_LINK} -outfile ${GLFW_ZIP}

#     Expand-Archive -Path ${GLFW_ZIP} -DestinationPath .
#     rm ${GLFW_ZIP}

#     echo "Building glfw"
#     cd ${GLFW_DIR}
#     md install
#     md build
#     cd build
#     cmake -L `
#       -D CMAKE_INSTALL_PREFIX="../install" `
#     ..
#     cmake --build . --parallel -j --config Release --target install

#   } else {
#     echo "glfw release exists"
#   }

#   # Copy all dependencies into a common tree and save it as cached
#   cd ${BUILD_DIR}

#   echo "Installing dependencies into a common place"
#   md ${DEPENDENCIES_DIR}
#   cp -Force -Recurse ${OSPRAY_DIR}/* ${DEPENDENCIES_DIR}
#   cp -Force -Recurse ${RKCOMMON_DIR}/install/* ${DEPENDENCIES_DIR}
#   cp -Force -Recurse ${GLFW_DIR}/install/* ${DEPENDENCIES_DIR}

#   echo "Creating dependencies archive for cached build"
#   Compress-Archive -DestinationPath ${DEPENDENCIES_ZIP} -Path ${DEPENDENCIES_DIR} 
#   # XXX Can macOS and Windows cache this on /NAS ???
#   # XXX Does Windows CI have a common drive mounted for this?

#   # Remove all but the installed dependencies
#   rm -Recurse ${OSPRAY_DIR}
#   rm -Recurse ${RKCOMMON_DIR}
#   rm -Recurse ${GLFW_DIR}
# }

cd ${BUILD_DIR}

echo "Building OSPRay Studio"
md -Force install
cmake -L `
  -D CMAKE_INSTALL_PREFIX="install" `
  -D ENABLE_OPENIMAGEIO=OFF `
  -D ENABLE_OPENVDB=OFF `
  -D ENABLE_EXR=OFF `
  ..
cmake --build . --parallel $env:NUMBER_OF_PROCESSORS --config Release --target install

popd
echo "done"
exit $LASTEXITCODE
