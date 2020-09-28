#!/bin/bash -x
## Copyright 2015-2020 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

set -e
apt-get update -y && apt-get install libglfw3-dev libxinerama-dev libxcursor-dev -y
if [[ ! -d "$CACHE_DIR/ospray-$OSPRAY_VER" ]]
then
    cd /tmp
    git clone https://gitlab-ci-token:${CI_JOB_TOKEN}@gitlab.com/sdvis/ospray.git ospray-$OSPRAY_VER
    cd ospray-$OSPRAY_VER/ && mkdir build && cd build && cmake ../scripts/superbuild -DBUILD_OIDN=ON && cmake --build .
    cp -r /tmp/ospray-$OSPRAY_VER $CACHE_DIR/
fi

cd $CI_PROJECT_DIR
mkdir build && cd build
export ospray_DIR=$CACHE_DIR/ospray-$OSPRAY_VER/build/install/ospray/lib/cmake/ospray-$OSPRAY_VER
export rkcommon_DIR=$CACHE_DIR/ospray-$OSPRAY_VER/build/install/rkcommon/lib/cmake/rkcommon-1.4.2
export TBB_ROOT=$CACHE_DIR/ospray-$OSPRAY_VER/build/tbb/src/tbb
cmake -DENABLE_OPENIMAGEIO=OFF -DENABLE_OPENVDB=OFF -DENABLE_EXR=OFF ..
make -j
