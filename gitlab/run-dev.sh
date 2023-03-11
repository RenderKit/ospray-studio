#!/bin/bash -x
## Copyright 2015 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

# XXX This script is transitional
# !!! This script only builds ospray and rkcommon devel branches !!!
# Override versions set by CI
OSPRAY_VER="devel"

set -e

# export DISPLAY=:1
# export USER=root
# mkdir -p $HOME/.vnc; echo testtest | vncpasswd -f > $HOME/.vnc/passwd; chmod 0600 $HOME/.vnc/passwd; touch $HOME/.vnc/xstartup; chmod +x $HOME/.vnc/xstartup
# vncserver $DISPLAY -geometry 1920x1080
# #glxinfo # informational only

export LD_LIBRARY_PATH=$CACHE_DIR/ospray-$OSPRAY_VER/build/install/lib:$LD_LIBRARY_PATH
cd ./build
ctest -N -VV  # list tests
ctest
set +e
timeout --preserve-status 10s ./ospStudio

set -e
# glTF 3D Commerce Certification Tests
export PATH=$(pwd):${PATH}
# ${CACHE_DIR}/glTF-Certification/run_cert.sh cert-tests

IMG_DIFF_TOOL=$CACHE_DIR/../tools/img_diff/img_diff
model_fns=(bunny.obj hairball.obj Peonies_2_obj.obj sponza.obj)
model_dirs=(bunny hairball Peonies sponza)
models=(bunny hairball Peonies sponza)
mse=(0.000001 0.000001 0.1 0.000001)
results="model-results"
for i in "${!models[@]}";do 
    ./ospStudio batch --format png --denoiser --spp 32 --forceRewrite \
        --resolution 1024x1024 --image ${results}/c-${models[i]} \
        $CACHE_DIR/datasets/${model_dirs[i]}/${model_fns[i]}
    echo "model ${model_dirs[i]}/${model_fns[i]} -> c-${models[i]} CI exit code $?"
    $IMG_DIFF_TOOL $CACHE_DIR/datasets/${models[i]}.png ${results}/c-${models[i]}.00000.png ${mse[i]}
    echo "MSE c-${models[i]} CI exit code $?"
done