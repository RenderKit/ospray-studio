#!/bin/bash -x
## Copyright 2015-2022 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

set -e

KW_SERVER_PATH=$KW_PATH/server
KW_CLIENT_PATH=$KW_PATH/client
export KLOCWORK_LTOKEN=/tmp/ltoken

echo "$KW_SERVER_IP;$KW_SERVER_PORT;$KW_USER;$KW_LTOKEN" > $KLOCWORK_LTOKEN

mkdir -p $CI_PROJECT_DIR/klocwork
log_file=$CI_PROJECT_DIR/klocwork/build.log

# # XXX This script is transitional
# # !!! This script only builds ospray and rkcommon devel branches !!!
# # Override versions set by CI
# OSPRAY_VER="devel"
# RKCOMMON_VER="devel"

# set -e
# dpkg --add-architecture i386
# apt-get update -y && apt-get install libglfw3-dev libxinerama-dev libxcursor-dev libtinfo5:i386 -y
# if [[ ! -d "$CACHE_DIR/ospray-$OSPRAY_VER" ]]
# then
#   build-dev.sh
# fi

cd $CI_PROJECT_DIR
mkdir build && cd build
# export CMAKE_PREFIX_PATH="$CACHE_DIR/ospray-$OSPRAY_VER/build/install"
# export TBB_ROOT=$CACHE_DIR/ospray-$OSPRAY_VER/build/tbb/src/tbb
# cmake -DENABLE_OPENIMAGEIO=OFF -DENABLE_OPENVDB=OFF -DENABLE_EXR=OFF ..

cmake -L \
  -D CMAKE_INSTALL_PREFIX=install \
  -D ENABLE_OPENIMAGEIO=OFF \
  -D ENABLE_OPENVDB=OFF \
  -D ENABLE_EXR=OFF \
  ..

$KW_CLIENT_PATH/bin/kwinject make -j8 | tee -a $log_file
$KW_SERVER_PATH/bin/kwbuildproject --url http://$KW_SERVER_IP:$KW_SERVER_PORT/$KW_PROJECT_NAME --tables-directory $CI_PROJECT_DIR/kw_tables kwinject.out | tee -a $log_file
$KW_SERVER_PATH/bin/kwadmin --url http://$KW_SERVER_IP:$KW_SERVER_PORT/ load --force --name build-$CI_JOB_ID $KW_PROJECT_NAME $CI_PROJECT_DIR/kw_tables | tee -a $log_file

# Store kw build name for check status later
echo "build-$CI_JOB_ID" > $CI_PROJECT_DIR/klocwork/build_name
