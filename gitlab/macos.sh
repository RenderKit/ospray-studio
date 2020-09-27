#!/bin/bash
## Copyright 2015-2020 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

# XXX Remove for gitlab
# OSPRAY_VER="2.3.0"
# RKCOMMON_VER="1.5.0"

# Pull OSPRay and RKCommon releases from github
GITHUB_HOME="https://github.com/ospray"

OSPRAY_DIR="ospray-${OSPRAY_VER}.x86_64.macosx"
OSPRAY_ZIP="${OSPRAY_DIR}.zip"
OSPRAY_LINK="${GITHUB_HOME}/ospray/releases/download/v${OSPRAY_VER}/${OSPRAY_ZIP}"

RKCOMMON_DIR="rkcommon-${RKCOMMON_VER}"
RKCOMMON_ZIP="v${RKCOMMON_VER}.zip"
RKCOMMON_LINK="${GITHUB_HOME}/rkcommon/archive/${RKCOMMON_ZIP}"

mkdir -p build-macos && cd build-macos
pushd .

cmake --version

# XXX Can macOS and Windows cache OSPRay and RKCommon on /NAS ???

if [ ! -d ${OSPRAY_DIR} ]; then
  echo "Fetching OSPRay release"
  wget --no-verbose ${OSPRAY_LINK}
  unzip -q ${OSPRAY_ZIP}
  rm ${OSPRAY_ZIP}
else
  echo "OSPRay release exists"
fi

if [ ! -d ${RKCOMMON_DIR} ]; then
  echo "Fetching RKCommon release"
  wget --no-verbose ${RKCOMMON_LINK}
  unzip -q ${RKCOMMON_ZIP}
  rm ${RKCOMMON_ZIP}
  cd ${RKCOMMON_DIR}
  # Need to build RKCommon
  mkdir build && cd build
  cmake ..  -DCMAKE_INSTALL_PREFIX=../install -DBUILD_TESTING=OFF
  make -j install
  cd .. && rm -rf build
else
  echo "RKCommon release exists"
fi

popd

CMAKE_PREFIX_PATH="${PWD}/${RKCOMMON_DIR}/install;${PWD}/${OSPRAY_DIR}"
cmake .. -DCMAKE_INSTALL_PREFIX=../install-macos \
  -DCMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH} \
  -DENABLE_OPENIMAGEIO=OFF -DENABLE_OPENVDB=OFF -DENABLE_EXR=OFF
make -j install
