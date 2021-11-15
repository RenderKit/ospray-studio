// Copyright 2009-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Generator.h"
#include "WaveletField.h"

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
  auto &tf = xfm.createChild("transferFunction", "transfer_function_turbo");

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
  volume.createChildData("data", dimensions, 0, voxels.data());

  const auto minmax = std::minmax_element(begin(voxels), end(voxels));
  auto valueRange = range1f(*std::get<0>(minmax), *std::get<1>(minmax));
  volume["valueRange"] = valueRange;
  // Although cell values lie outside (0, 1), leave the default transfer
  // function range.  It's an interesting image.
  tf["valueRange"] = vec2f(0.f, 1.f);

}

} // namespace sg
} // namespace ospray
