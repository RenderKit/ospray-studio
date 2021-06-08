#!/usr/bin/env pwsh
## Copyright 2009-2021 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

$ErrorActionPreference = "Stop"

$PACKAGE = "OFF"
if ($args.Length -gt 0) {
  if ($($args[0]) -eq "package") {
    $PACKAGE = "ON"
  }
}

cmake --version

mkdir -Force build-win
Set-Location build-win

if ($PACKAGE -eq "ON") {
  cmake -L `
    -D ENABLE_OPENIMAGEIO=OFF `
    -D ENABLE_OPENVDB=OFF `
    -D ENABLE_EXR=OFF `
    -D OSPSTUDIO_SIGN_FILE=$env:SIGN_FILE_WINDOWS `
    ..
  cmake --build . --parallel --config Release
  cpack -B "${PWD}/package" -V
  cpack -G ZIP -B "${PWD}/package" -V
} else {
  cmake -L `
    -D CMAKE_INSTALL_PREFIX="install" `
    -D ENABLE_OPENIMAGEIO=OFF `
    -D ENABLE_OPENVDB=OFF `
    -D ENABLE_EXR=OFF `
    ..
  cmake --build . --parallel --config Release --target install
}

exit $LASTEXITCODE
