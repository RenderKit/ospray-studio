#!/bin/bash -x
## Copyright 2015-2020 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

# XXX This script is transitional
# !!! This script only builds ospray and rkcommon devel branches !!!
# Override veresions set by CI
OSPRAY_VER="devel"

set -e
apt-get update -y && apt-get install libglfw3-dev libxinerama-dev libxcursor-dev -y

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

export LD_LIBRARY_PATH=$CACHE_DIR/ospray-$OSPRAY_VER/build/install/ospray/lib/:$CACHE_DIR/ospray-$OSPRAY_VER/build/install/oidn/lib/:$LD_LIBRARY_PATH
cd $CI_PROJECT_DIR/build
set +e
timeout --preserve-status 10s ./ospStudio
echo "Last exit code $?"
