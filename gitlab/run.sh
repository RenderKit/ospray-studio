#!/bin/bash -x
## Copyright 2015 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

set -e

export DISPLAY=:1
export USER=root
mkdir -p $HOME/.vnc; echo testtest | vncpasswd -f > $HOME/.vnc/passwd; chmod 0600 $HOME/.vnc/passwd; touch $HOME/.vnc/xstartup; chmod +x $HOME/.vnc/xstartup
vncserver $DISPLAY -geometry 1920x1080
glxinfo

LD_LIBRARY_PATH=${PWD:?}/build/install/lib${LD_LIBRARY_PATH:+:${LD_LIBRARY_PATH:?}}
export LD_LIBRARY_PATH

cd ./build
set +e

timeout --preserve-status 10s ./ospStudio
exitCode=$?

echo "timeout originally returned: ${exitCode:?}"

if [ "${exitCode:?}" -eq 143 ]; then
    echo "code 143 is SIGTERM, which we expect from terminating the studio GUI"
    exitCode=0
fi

echo "Exiting with code: ${exitCode:?}"
exit "${exitCode:?}"
