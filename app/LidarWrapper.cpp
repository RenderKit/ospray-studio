// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "LidarWrapper.h"
#include "valeo_lidar.h"

// XXX those should not be needed here
#define NUM_APD_GROUPS 20    // Number of APD Groups
#define NUM_APDS_PER_GROUP 4 // Number of (vertically stacked) APDs in one group
#define NUM_COLUMNS 1440
#define POSTPRO_INPUT_HORIZONTAL_PIXELS_PER_LAYER (NUM_COLUMNS * 6)
#define POSTPRO_INPUT_VERTICAL_PIXELS_PER_LAYER (NUM_APD_GROUPS * NUM_APDS_PER_GROUP * 32)

void LidarProcessFrame(
    const float depths[LIDAR_FRAMEBUFFER_HEIGHT][LIDAR_FRAMEBUFFER_WIDTH],
    const float rgba[LIDAR_FRAMEBUFFER_HEIGHT][LIDAR_FRAMEBUFFER_WIDTH][4],
    const int verbose,
    const char *file_name)
{
  static_assert(
      POSTPRO_INPUT_HORIZONTAL_PIXELS_PER_LAYER == LIDAR_FRAMEBUFFER_WIDTH,
      "FB width differs");
  static_assert(
      POSTPRO_INPUT_VERTICAL_PIXELS_PER_LAYER == LIDAR_FRAMEBUFFER_HEIGHT,
      "FB height differs");

  auto distance = new double[LIDAR_FRAMEBUFFER_HEIGHT][LIDAR_FRAMEBUFFER_WIDTH];
  double *pDist[LIDAR_FRAMEBUFFER_HEIGHT];
  for (int y = 0; y < POSTPRO_INPUT_VERTICAL_PIXELS_PER_LAYER; y++) {
    pDist[y] = distance[y];
    for (int x = 0; x < POSTPRO_INPUT_HORIZONTAL_PIXELS_PER_LAYER; x++)
      distance[LIDAR_FRAMEBUFFER_HEIGHT - 1 - y][x] = depths[y][x];
  }

  auto intensity = new double[LIDAR_FRAMEBUFFER_HEIGHT][LIDAR_FRAMEBUFFER_WIDTH];
  double *pInt[LIDAR_FRAMEBUFFER_HEIGHT];
  for (int y = 0; y < POSTPRO_INPUT_VERTICAL_PIXELS_PER_LAYER; y++) {
    pInt[y] = intensity[y];
    for (int x = 0; x < POSTPRO_INPUT_HORIZONTAL_PIXELS_PER_LAYER; x++)
      intensity[LIDAR_FRAMEBUFFER_HEIGHT - 1 - y][x] =
          rgba[y][x][0] + rgba[y][x][1] * 0.1f + rgba[y][x][2] * 0.01f;
  }

  processFrame(pDist,
      pInt,
      NUM_COLUMNS,
      NUM_APDS_PER_GROUP,
      NUM_APD_GROUPS,
      verbose,
      file_name);

  delete[] distance;
  delete[] intensity;
};
