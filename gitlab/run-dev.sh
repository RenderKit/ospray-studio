#!/bin/bash -x
## Copyright 2015 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

set -e

export DISPLAY=:1
export USER=root
mkdir -p $HOME/.vnc; echo testtest | vncpasswd -f > $HOME/.vnc/passwd; chmod 0600 $HOME/.vnc/passwd; touch $HOME/.vnc/xstartup; chmod +x $HOME/.vnc/xstartup
vncserver $DISPLAY -geometry 1920x1080
#glxinfo # informational only

export PATH=.:${PATH}
LD_LIBRARY_PATH=${PWD:?}/build/install/lib${LD_LIBRARY_PATH:+:${LD_LIBRARY_PATH:?}}
export LD_LIBRARY_PATH

cd ./build
ctest -N -VV  # list tests
ctest
set +e
timeout --preserve-status 10s ./ospStudio

set -e
# glTF 3D Commerce Certification Tests
${STORAGE_PATH}/ci-cache/glTF-Certification/run_cert.sh cert-tests
exit $?
