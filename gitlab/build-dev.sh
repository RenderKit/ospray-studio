#!/bin/bash -x
## Copyright 2015-2020 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

# XXX This script is transitional
# !!! This script only builds ospray and rkcommon devel branches !!!
# Override veresions set by CI
OSPRAY_VER="devel"
RKCOMMON_VER="devel"

# Need a way to clean and force rebuild "ospray-devel" cache on a change to dependent branch
# Ultimately, OSPRay CI is responsible for building OSPRay!  Studio should just use those artifacts.
set -e
apt-get update -y && apt-get install libglfw3-dev libxinerama-dev libxcursor-dev -y
if [[ ! -d "$CACHE_DIR/ospray-$OSPRAY_VER" ]]
then
    cd /tmp
    git clone http://gitlab-ci-token:${CI_JOB_TOKEN}@$CI_SERVER_HOST/renderkit/ospray.git ospray-$OSPRAY_VER
    cd ospray-$OSPRAY_VER/
    git checkout ${OSPRAY_VER}
    mkdir build && cd build
    cmake -L \
      -DBUILD_OIDN=ON \
      -DBUILD_RKCOMMON_VERSION=devel \
      -DINSTALL_IN_SEPARATE_DIRECTORIES=OFF \
      "$@" \
      ../scripts/superbuild
    cmake --build .
    cp -r /tmp/ospray-$OSPRAY_VER $CACHE_DIR/
fi

cd $CI_PROJECT_DIR
mkdir build && cd build
export CMAKE_PREFIX_PATH="$CACHE_DIR/ospray-$OSPRAY_VER/build/install"
#export CMAKE_PREFIX_PATH="$CACHE_DIR/ospray-$OSPRAY_VER/build/install/ospray:$CACHE_DIR/ospray-$OSPRAY_VER/build/install/rkcommon"
#export ospray_DIR=$CACHE_DIR/ospray-$OSPRAY_VER/build/install/ospray/lib/cmake/ospray-$OSPRAY_VER
#export rkcommon_DIR=$CACHE_DIR/ospray-$OSPRAY_VER/build/install/rkcommon/lib/cmake
export TBB_ROOT=$CACHE_DIR/ospray-$OSPRAY_VER/build/tbb/src/tbb
cmake -DENABLE_OPENIMAGEIO=OFF -DENABLE_OPENVDB=OFF -DENABLE_EXR=OFF ..
make -j
