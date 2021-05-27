// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Generator.h"
// std
#include <algorithm>
#include <random>
// rkcommon
#include "rkcommon/tasking/parallel_for.h"

namespace ospray {
namespace sg {

struct Torus : public Generator
{
  Torus();
  ~Torus() override = default;

  void generateData() override;
};

OSP_REGISTER_SG_NODE_NAME(Torus, generator_torus_volume);

// Torus definitions ////////////////////////////////////////////////

Torus::Torus()
{
  auto &parameters = child("parameters");
  parameters.createChild("size", "int", "cubic volume, size^3", 256);
  parameters.createChild("R", "float", "outer radius", 80.f);
  parameters.createChild("r", "float", "thickness", 30.f);
  parameters.child("size").setMinMax(10, 512);
  parameters.child("r").setMinMax(5.f, 512.f);
  parameters.child("R").setMinMax(5.f, 512.f);

  auto &xfm = createChild("xfm", "transform");
}

void Torus::generateData()
{
  auto &parameters = child("parameters");

  // Maintain a persistent volume, don't recreate on each generation
  static std::vector<float> volumetricData;
  static auto size = 0;
  if (size != parameters["size"].valueAs<int>()) {
    size = parameters["size"].valueAs<int>();
    volumetricData.resize(size * size * size, 0);
  }

  auto r = parameters["r"].valueAs<float>();
  auto R = parameters["R"].valueAs<float>();

  auto &xfm = child("xfm");
  auto &tf = xfm.createChild("transfer_function", "transfer_function_jet");

  const int size_2 = size / 2;

  tasking::parallel_for(size, [&](int x) {
    for (int y = 0; y < size; ++y) {
      for (int z = 0; z < size; ++z) {
        const float X = x - size_2;
        const float Y = y - size_2;
        const float Z = z - size_2;

        const float d = (R - std::sqrt(X * X + Y * Y));
        volumetricData[size * size * x + size * y + z] = r * r - d * d - Z * Z;
      }
    }
  });

  auto volume = createNode("torus", "structuredRegular");
  volume->createChild("voxelType", "int", int(OSP_FLOAT));
  volume->createChild("gridOrigin", "vec3f", vec3f(-0.5f));
  volume->createChild("gridSpacing", "vec3f", vec3f(1.f / size));
  // The "true" flag shares data with OSPRay rather than copying.
  volume->createChildData("data", vec3i(size), 0, volumetricData.data(), true);

  // The torus is prominent in the 0.f - 1.f value range.
  auto valueRange = range1f(0.f, 1.f);
  volume->child("valueRange") = valueRange;

  tf.add(volume);
  tf["valueRange"] = vec2f(0.f, 1.f);
}

} // namespace sg
} // namespace ospray
