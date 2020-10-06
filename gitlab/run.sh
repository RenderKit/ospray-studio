#!/bin/bash -x
## Copyright 2009-2020 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

set -e
apt-get update -y && apt-get install libglfw3-dev libxinerama-dev libxcursor-dev -y
if [[ ! -d "$CACHE_DIR/ospray-2.2.0" ]]
then
    cd tmp
    wget https://github.com/ospray/OSPRay/archive/v2.2.0.tar.gz
    tar -xvf v2.2.0.tar.gz --no-same-owner
    rm v2.2.0.tar.gz
    cd ospray-2.2.0/ && mkdir build && cd build && cmake ../scripts/superbuild -DBUILD_OIDN=ON && cmake --build .
    cp -r ospray-2.2.0 $CACHE_DIR/
fi
export DEBIAN_FRONTEND=noninteractive
apt update
apt install -y xorg
apt install -y mesa-utils
apt install -y tigervnc-standalone-server
export DISPLAY=:1
export USER=root
mkdir -p $HOME/.vnc; echo testtest | vncpasswd -f > $HOME/.vnc/passwd; chmod 0600 $HOME/.vnc/passwd; touch $HOME/.vnc/xstartup; chmod +x $HOME/.vnc/xstartup
vncserver $DISPLAY -geometry 1900x1000
glxinfo

export LD_LIBRARY_PATH=$CACHE_DIR/ospray-2.2.0/build/install/ospray/lib/:$CACHE_DIR/ospray-2.2.0/build/install/oidn/lib/:$LD_LIBRARY_PATH
cd $CI_PROJECT_DIR/build
set +e
timeout --preserve-status 10s ./ospStudio
echo "Last exit code $?"
