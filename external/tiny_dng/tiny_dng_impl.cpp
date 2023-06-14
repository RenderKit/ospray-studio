// Copyright 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// Define TINY_DNG_LOADER_IMPLEMENTATION and STB_IMAGE_IMPLEMENTATION in only one *.cc
#define TINY_DNG_LOADER_USE_THREAD
//#define TINY_DNG_LOADER_NO_STB_IMAGE_INCLUDE
#define TINY_DNG_LOADER_IMPLEMENTATION
#include "tiny_dng_loader.h"
