#!/bin/bash -ex
## Copyright 2015 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

PACKAGE=false
if [[ $# -gt 0 ]]
then
    if [[ "$1" == "package" ]]
    then
        PACKAGE=true
    fi
fi

THREADS=`sysctl -n hw.logicalcpu`

cmake --version

mkdir -p build-macos

cd build-macos

if [[ "$PACKAGE" == true ]]
then
    cmake -L \
        -D CMAKE_INSTALL_PREFIX="" \
        -D ENABLE_OPENIMAGEIO=OFF \
        -D ENABLE_OPENVDB=OFF \
        -D ENABLE_EXR=OFF \
        -D CMAKE_BUILD_WITH_INSTALL_RPATH=ON \
        -D CPACK_PRE_BUILD_SCRIPTS=$CPACK_PRE_BUILD_SIGNING_SCRIPT \
        ..
    cmake --build . --parallel $THREADS --config Release
    cpack -G ZIP -B "${PWD}/package"

    cmake -L \
        -D CMAKE_INSTALL_PREFIX=/opt/local \
        -D CMAKE_INSTALL_DOCDIR=../../Applications/OSPRay/doc \
        -D CMAKE_INSTALL_BINDIR=../../Applications/OSPRay/bin \
        -D ENABLE_OPENIMAGEIO=OFF \
        -D ENABLE_OPENVDB=OFF \
        -D ENABLE_EXR=OFF \
        -D CMAKE_BUILD_WITH_INSTALL_RPATH=ON \
        -D CPACK_PRE_BUILD_SCRIPTS=$CPACK_PRE_BUILD_SIGNING_SCRIPT \
        ..
    cmake --build . --parallel $THREADS --config Release
    cpack -B "${PWD}/package"
else
    cmake -L \
        -D CMAKE_INSTALL_PREFIX=install \
        -D ENABLE_OPENIMAGEIO=OFF \
        -D ENABLE_OPENVDB=OFF \
        -D ENABLE_EXR=OFF \
        ..
    cmake --build . --parallel $THREADS --config Release --target install
fi

cd ..
