// Copyright 2009-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Generator.h"
// std
#include <random>
// rkcommon
#include "rkcommon/tasking/parallel_for.h"

namespace ospray {
namespace sg {

struct WaveletVolume : public Generator
{
  WaveletVolume();
  ~WaveletVolume() override = default;

  void generateData() override;
};

OSP_REGISTER_SG_NODE_NAME(WaveletVolume, generator_wavelet);

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

// WaveletVolume definitions ////////////////////////////////////////////////

WaveletVolume::WaveletVolume()
{
  auto &parameters = child("parameters");

  parameters.createChild("dimensions", "vec3i", vec3i(128));
  parameters.createChild("gridOrigin", "vec3f", vec3f(-1.f));
  parameters.createChild("gridSpacing", "vec3f", vec3f(2.f / 100));

  auto &xfm = createChild("xfm", "transform");
}

void WaveletVolume::generateData()
{
  auto &xfm = child("xfm");
  auto &tf = xfm.createChild("transferFunction", "transfer_function_jet");

  // Create voxel data

  auto &parameters = child("parameters");

  auto dimensions = parameters["dimensions"].valueAs<vec3i>();
  auto gridOrigin = parameters["gridOrigin"].valueAs<vec3f>();
  auto gridSpacing = parameters["gridSpacing"].valueAs<vec3f>();

  std::vector<float> voxels(dimensions.long_product());

  auto transformLocalToObject = [&](const vec3f &localCoordinates) {
    return gridOrigin + localCoordinates * gridSpacing;
  };

  tasking::parallel_for(dimensions.z, [&](int z) {
    for (size_t y = 0; y < (size_t)dimensions.y; y++) {
      for (size_t x = 0; x < (size_t)dimensions.x; x++) {
        size_t index =
            z * (size_t)dimensions.y * dimensions.x + y * dimensions.x + x;
        vec3f objectCoordinates = transformLocalToObject(vec3f(x, y, z));
        voxels[index] = getWaveletValue(objectCoordinates);
      }
    }
  });

  // Create sg subtree
  auto &volume = tf.createChild("wavelet", "structuredRegular");
  volume.createChild("voxelType") = int(OSP_FLOAT);
  volume.createChild("gridOrigin") = gridOrigin;
  volume.createChild("gridSpacing") = gridSpacing;
  volume.createChild("dimensions") = dimensions;
  volume.createChildData("data", dimensions, 0, voxels.data());
}

} // namespace sg
} // namespace ospray
