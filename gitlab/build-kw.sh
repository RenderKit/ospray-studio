#!/bin/bash -x
KW_SERVER_PATH=$KW_PATH/server
KW_CLIENT_PATH=$KW_PATH/client
export KLOCWORK_LTOKEN=/tmp/ltoken

echo "$KW_SERVER_IP;$KW_SERVER_PORT;$KW_USER;$KW_LTOKEN" > $KLOCWORK_LTOKEN

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

$KW_CLIENT_PATH/bin/kwinject make -j8 
$KW_SERVER_PATH/bin/kwbuildproject --url http://$KW_SERVER_IP:$KW_SERVER_PORT/$KW_PROJECT_NAME --tables-directory $CI_PROJECT_DIR/kw_tables kwinject.out
$KW_SERVER_PATH/bin/kwadmin --url http://$KW_SERVER_IP:$KW_SERVER_PORT/ load --force --name build-$CI_JOB_ID $KW_PROJECT_NAME $CI_PROJECT_DIR/kw_tables
echo "build-$CI_JOB_ID" > $CI_PROJECT_DIR/kw_build_number

