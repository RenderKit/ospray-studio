// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// Althought stb_image.h is in the external directory, there can be only one
// implementation in the entire scene graph.  Implement it here.

#define STB_IMAGE_IMPLEMENTATION // define this in only *one* .cc
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION // define this in only *one* .cc
#include "stb_image_write.h"
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize.h"
