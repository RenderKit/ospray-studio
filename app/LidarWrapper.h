// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0
#ifndef LIDARWRAPPER_H
#define LIDARWRAPPER_H

#include "valeo_lidar.h"

#pragma once



// The following can be adapted within certain ranges:
// NUM_APD_GROUPS * NUM_APDS_PER_GROUP has a maximum of 200
// NUM_COLUMNS has a maximum of 2880

#define NUM_APD_GROUPS 20    // Number of APD Groups
#define NUM_APDS_PER_GROUP 4 // Number of (vertically stacked) APDs in one group
#define NUM_COLUMNS 1440


// DO NOT CHANGE ANY OF THE FOLLOWING

#define NUM_APDS (NUM_APD_GROUPS * NUM_APDS_PER_GROUP)

#define HORIZ_FOV_DEG (static_cast<double>(NUM_COLUMNS) * static_cast<double>(APD_HORIZ_APERTURE_DEG))
#define VERTI_FOV_DEG (static_cast<double>(NUM_APD_GROUPS) * static_cast<double>(NUM_APDS_PER_GROUP-1) * (static_cast<double>(APD_VERTI_PICKUP_RANGE_DEG)+static_cast<double>(APD_VERTI_SEPARATION_DEG)) + \
    static_cast<double>(NUM_APD_GROUPS-1) * (static_cast<double>(APD_VERTI_PICKUP_RANGE_DEG)+static_cast<double>(APDGROUP_VERTI_SEPARATION_DEG)) + \
    static_cast<double>(APD_VERTI_PICKUP_RANGE_DEG))


#define LIDAR_FRAMEBUFFER_WIDTH (NUM_COLUMNS * APD_AV_WIDTH_PIXEL)	// horiz. number of pixels for postprocessing input
#define LIDAR_FRAMEBUFFER_HEIGHT (NUM_APDS * APD_AV_HEIGHT_PIXEL) // vertical number of pixels for postprocessing input

#define LIDAR_IMAGE_START_HOR ((360.0f - static_cast<double>(HORIZ_FOV_DEG))/2.0f/360.0f)
#define LIDAR_IMAGE_END_HOR  ((360.f + static_cast<double>(HORIZ_FOV_DEG))/2.f/360.f)
#define LIDAR_IMAGE_START_VER  ((180.f - static_cast<double>(VERTI_FOV_DEG))/2.f/180.f)
#define LIDAR_IMAGE_END_VER  ((180.f + static_cast<double>(VERTI_FOV_DEG))/2.f/180.f)

#define LIDAR_IMAGE_START (LIDAR_IMAGE_START_HOR, LIDAR_IMAGE_START_VER)
#define LIDAR_IMAGE_END (LIDAR_IMAGE_END_HOR, LIDAR_IMAGE_END_VER)





extern "C" void LidarProcessFrame(
    const float depths[LIDAR_FRAMEBUFFER_HEIGHT][LIDAR_FRAMEBUFFER_WIDTH],
    const float rgba[LIDAR_FRAMEBUFFER_HEIGHT][LIDAR_FRAMEBUFFER_WIDTH][4],
    const int verbose,
    const char *file_name);
#endif
