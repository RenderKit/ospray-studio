#!/bin/bash
## Copyright 2015 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

# XXX This script is transitional
# !!! This script only builds rkcommon devel branch and ospray *non-devel* !!!
# Override versions set by CI
RKCOMMON_VER="devel"

# Stop script on first failure
set -e

# Versions if not set by CI (standalone run)
OSPRAY_VER=${OSPRAY_VER:-"2.6.0"}
RKCOMMON_VER=${RKCOMMON_VER:-"1.6.1"}
GLFW_VER=${GLFW_VER:-"3.3.2"}

# Pull OSPRay and RKCommon releases from github
GITHUB_HOME="https://github.com/ospray"

OSPRAY_DIR="ospray-${OSPRAY_VER}.x86_64.macosx"
OSPRAY_ZIP="${OSPRAY_DIR}.zip"
OSPRAY_LINK="${GITHUB_HOME}/ospray/releases/download/v${OSPRAY_VER}/${OSPRAY_ZIP}"

RKCOMMON_DIR="rkcommon-${RKCOMMON_VER}"
RKCOMMON_ZIP="${RKCOMMON_VER}.zip"
RKCOMMON_LINK="${GITHUB_HOME}/rkcommon/archive/${RKCOMMON_ZIP}"

THREADS=`sysctl -n hw.logicalcpu`

mkdir -p build-macos && cd build-macos
pushd .

cmake --version

# XXX Can macOS and Windows cache OSPRay and RKCommon on /NAS ???

# if [ ! -d ${OSPRAY_DIR} ]; then
#   echo "Fetching OSPRay release ${OSPRAY_VER}"
#   wget --no-verbose ${OSPRAY_LINK}
#   unzip -q ${OSPRAY_ZIP}
#   rm ${OSPRAY_ZIP}
# else
#   echo "OSPRay release exists"
# fi

# if [ ! -d ${RKCOMMON_DIR} ]; then
#   echo "Fetching RKCommon release ${RKCOMMON_VER}"
#   wget --no-verbose ${RKCOMMON_LINK}
#   unzip -q ${RKCOMMON_ZIP}
#   rm ${RKCOMMON_ZIP}
#   cd ${RKCOMMON_DIR}
#   # Need to build RKCommon
#   echo "Building RKCommon"
#   mkdir build && cd build
#   cmake ..  -DCMAKE_INSTALL_PREFIX=../install -DBUILD_TESTING=OFF
#   make -j install
#   cd .. && rm -rf build
# else
#   echo "RKCommon release exists"
# fi

popd

# CMAKE_PREFIX_PATH="${PWD}/${RKCOMMON_DIR}/install;${PWD}/${OSPRAY_DIR}"
cmake .. -DCMAKE_INSTALL_PREFIX=install \
  -DENABLE_OPENIMAGEIO=OFF -DENABLE_OPENVDB=OFF -DENABLE_EXR=OFF
make -j $THREADS install
