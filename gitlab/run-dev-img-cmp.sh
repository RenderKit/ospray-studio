#!/bin/bash -x
## Copyright 2015 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

set -e

OSPRAY_VER="devel"

export LD_LIBRARY_PATH=$CACHE_DIR/ospray-$OSPRAY_VER/build/install/lib:$LD_LIBRARY_PATH

SCRIPT_DIR=$(pwd)/$(dirname "$0")
echo $SCRIPT_DIR

cd ./build
#IMG_DIFF_TOOL=$CACHE_DIR/../tools/img_diff/img_diff
pip3 install --user --no-warn-script-location scikit-image argparse numpy sewar reportlab imagecodecs
model_fns=(bunny.obj hairball.obj Peonies_2_obj.obj sponza.sg)
model_dirs=(bunny hairball Peonies sponza)
models=(bunny hairball Peonies sponza)
mse=(0.000001 0.000001 0.1 0.000001)
results="model-results"

mkdir -p ${results}
for i in "${!models[@]}";do 
    ./ospStudio batch --format png --denoiser --spp 32 --forceRewrite \
        --resolution 1024x1024 --image ${results}/c-${models[i]} \
        $CACHE_DIR/datasets/${model_dirs[i]}/${model_fns[i]}
    echo "model ${model_dirs[i]}/${model_fns[i]} -> c-${models[i]} CI exit code $?"
    #$IMG_DIFF_TOOL $CACHE_DIR/datasets/${models[i]}.png ${results}/c-${models[i]}.00000.png ${mse[i]}
    #echo "MSE c-${models[i]} CI exit code $?"
    set -e 
    python3 $SCRIPT_DIR/image-comparison.py --reference $CACHE_DIR/datasets/${models[i]}.png --candidate ${results}/c-${models[i]}.00000.png --mse ${mse[i]}
done