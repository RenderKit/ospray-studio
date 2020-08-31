// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <vector>
#include "rkcommon/vec.h"
#include "rkcommon/FileName.h"

#ifdef _WIN32
  #ifdef ospray_tfn_EXPORTS
    #define OSPTFNLIB_INTERFACE __declspec(dllexport)
  #else
    #define OSPTFNLIB_INTERFACE __declspec(dllimport)
  #endif
#else
  #define OSPTFNLIB_INTERFACE
#endif

/* The transfer function file format used by the OSPRay sample apps is a
 * little endian binary format with the following layout:
 * uint32: magic number identifying the file
 * uint64: version number
 * uint64: length of the name of the transfer function (not including \0)
 * [char...]: name of the transfer function (without \0)
 * uint64: number of vec3f color values
 * uint64: numer of vec2f data value, opacity value pairs
 * float64: data value min
 * float64: data value max
 * float32: opacity scaling value, opacity values should be scaled
 *          by this factor
 * [rkcommon::vec3f...]: RGB values
 * [rkcommon::vec2f...]: data value, opacity value pairs
 */

namespace tfn {

struct OSPTFNLIB_INTERFACE TransferFunction {
  std::string name;
  std::vector<rkcommon::vec3f> rgbValues;
  std::vector<rkcommon::vec2f> opacityValues;
  double dataValueMin;
  double dataValueMax;
  float opacityScaling;

  // Load the transfer function data in the file
  TransferFunction(const rkcommon::FileName &fileName);
  // Construct a transfer function from some existing one
  TransferFunction(const std::string &name,
      const std::vector<rkcommon::vec3f> &rgbValues,
      const std::vector<rkcommon::vec2f> &opacityValues, const double dataValueMin,
      const double dataValueMax, const float opacityScaling);
  // Save the transfer function data to the file
  void save(const rkcommon::FileName &fileName) const;
};

}

