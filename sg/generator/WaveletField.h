// Copyright 2009-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Generator.h"
// std
#include <random>
// rkcommon
#include "rkcommon/tasking/parallel_for.h"

namespace ospray {
namespace sg {

template <typename VOXEL_TYPE = float>
inline VOXEL_TYPE getWaveletValue(const vec3f &objectCoordinates)
{
  // wavelet parameters
  constexpr double M = 1.f;
  constexpr double G = 1.f;
  constexpr double XM = 1.f;
  constexpr double YM = 1.f;
  constexpr double ZM = 1.f;
  constexpr double XF = 3.f;
  constexpr double YF = 3.f;
  constexpr double ZF = 3.f;

  double value = M * G
      * (XM * std::sin(XF * objectCoordinates.x)
          + YM * std::sin(YF * objectCoordinates.y)
          + ZM * std::cos(ZF * objectCoordinates.z));

  if (std::is_unsigned<VOXEL_TYPE>::value) {
    value = fabs(value);
  }

  value = clamp(value,
      double(std::numeric_limits<VOXEL_TYPE>::lowest()),
      double(std::numeric_limits<VOXEL_TYPE>::max()));

  return VOXEL_TYPE(value);
}

} // namespace sg
} // namespace ospray
