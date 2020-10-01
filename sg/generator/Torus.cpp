// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Generator.h"
// std
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

  Torus::Torus() {}

  void Torus::generateData()
  {
    int size = 256;
    std::vector<float> volumetricData(size * size * size, 0);

    const float r    = 30;
    const float R    = 80;
    const int size_2 = size / 2;

    for (int x = 0; x < size; ++x) {
      for (int y = 0; y < size; ++y) {
        for (int z = 0; z < size; ++z) {
          const float X = x - size_2;
          const float Y = y - size_2;
          const float Z = z - size_2;

          const float d = (R - std::sqrt(X * X + Y * Y));
          volumetricData[size * size * x + size * y + z] =
              r * r - d * d - Z * Z;
        }
      }
    }

    auto &tf = createChild("transfer_function", "transfer_function_jet");
    auto volume = createNode("torus", "structuredRegular");
    volume->createChild("voxelType", "int", int(OSP_FLOAT));
    volume->createChild("gridOrigin", "vec3f", vec3f(-0.5f, -0.5f, -0.5f));
    volume->createChild(
        "gridSpacing", "vec3f", vec3f(1.f / size, 1.f / size, 1.f / size));
    volume->createChild("dimensions", "vec3i", vec3i(size));
    volume->createChildData("data", vec3i(size), 0, volumetricData.data());
    tf.add(volume);
  }

  }  // namespace sg
} // namespace ospray
