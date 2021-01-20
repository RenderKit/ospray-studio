// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#define LIDAR_FRAMEBUFFER_WIDTH 8640
#define LIDAR_FRAMEBUFFER_HEIGHT 2560
#define LIDAR_IMAGE_START (0.25f, 0.352772f)
#define LIDAR_IMAGE_END (0.75f, 0.647228f)

extern "C" void LidarProcessFrame(
    const float depths[LIDAR_FRAMEBUFFER_HEIGHT][LIDAR_FRAMEBUFFER_WIDTH],
    const float rgba[LIDAR_FRAMEBUFFER_HEIGHT][LIDAR_FRAMEBUFFER_WIDTH][4],
    const int verbose,
    const char *file_name);
