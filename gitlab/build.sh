#!/bin/bash -x
## Copyright 2015-2021 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

set -e
apt-get update -y && apt-get install libglfw3-dev libxinerama-dev libxcursor-dev -y
if [[ ! -d "$CACHE_DIR/ospray-$OSPRAY_VER" ]]
then
    cd /tmp
    git clone http://gitlab-ci-token:${CI_JOB_TOKEN}@$CI_SERVER_HOST/renderkit/ospray.git ospray-$OSPRAY_VER
    cd ospray-$OSPRAY_VER/
    git checkout tags/"v${OSPRAY_VER}"
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
export TBB_ROOT=$CACHE_DIR/ospray-$OSPRAY_VER/build/tbb/src/tbb
cmake -DENABLE_OPENIMAGEIO=OFF -DENABLE_OPENVDB=OFF -DENABLE_EXR=OFF ..
make -j
