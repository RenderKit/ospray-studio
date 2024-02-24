#!/bin/bash -x
## Copyright 2015 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

set -e

export PATH=.:${PATH}
LD_LIBRARY_PATH=${PWD:?}/build/install/lib${LD_LIBRARY_PATH:+:${LD_LIBRARY_PATH:?}}
export LD_LIBRARY_PATH

SCRIPT_DIR=$(pwd)/$(dirname "$0")
echo $SCRIPT_DIR

cd ./build
pip3 install --user --no-warn-script-location scikit-image argparse numpy sewar reportlab imagecodecs
model_fns=(bunny.sg hairball.sg Peonies.sg sponza.sg)
model_dirs=(bunny hairball Peonies sponza)
models=(bunny hairball Peonies sponza)
cam_cnt=(2 1 1 1)

uqi=(0.999 0.999 0.999 0.999)
results="model-results"

data_path=${STORAGE_PATH}/ci-cache/datasets

mkdir -p ${results}
for i in "${!models[@]}";do 
    #copy cameras file to app location
    cp ${data_path}/${model_dirs[i]}/cams.json .

    ./ospStudio batch --format png --denoiser --spp 32 \
        --resolution 1024x1024 --image ${results}/c-${models[i]} \
        --loadCams cams.json \
        ${data_path}/${model_dirs[i]}/${model_fns[i]}

    rm cams.json
    echo "model ${model_dirs[i]}/${model_fns[i]} -> c-${models[i]} CI exit code $?"
    #echo "MSE c-${models[i]} CI exit code $?"

    #using an sg means it can't have --forceRewrite, always move 00000 to -----
    set -e 
    for ((j=0; j<${cam_cnt[i]}; j++));
    do
        mv ${results}/c-${models[i]}.0000$j.png ${results}/c-${models[i]}.----$j.png
        md5sum ${data_path}/${models[i]}/${models[i]}_$j\_gold.png
        python3 $SCRIPT_DIR/image-comparison.py --reference ${data_path}/${models[i]}/${models[i]}_$j\_gold.png --candidate ${results}/c-${models[i]}.----$j.png --uqi ${uqi[i]}
    done

done
