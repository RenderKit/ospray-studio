// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Generator.h"
// std
#include <random>

namespace ospray {
  namespace sg {

  struct RandomSpheres : public Generator
  {
    RandomSpheres()           = default;
    ~RandomSpheres() override = default;

    void generateData() override;
  };

  OSP_REGISTER_SG_NODE_NAME(RandomSpheres, generator_random_spheres);

  // RandomSpheres definitions ////////////////////////////////////////////////

  void RandomSpheres::generateData()
  {
    remove("spheres");

    const int numSpheres = 1e6;
    const float radius   = 0.002f;

    auto &spheres = createChild("spheres", "geometry_spheres");

    std::mt19937 rng(0);
    std::uniform_real_distribution<float> dist(-1.f + radius, 1.f - radius);

    std::vector<vec3f> centers;

    for (int i = 0; i < numSpheres; ++i)
      centers.push_back(vec3f(dist(rng), dist(rng), dist(rng)));

    spheres.createChildData("sphere.position", centers);
    spheres.child("radius") = radius;
  }

  }  // namespace sg
} // namespace ospray
