// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Generator.h"
// std
#include <random>

namespace ospray {
  namespace sg {

  struct Sphere : public Generator
  {
    Sphere()           = default;
    ~Sphere() override = default;

    void generateData() override;
  };

  OSP_REGISTER_SG_NODE_NAME(Sphere, generator_sphere);

  // Sphere definitions ////////////////////////////////////////////////

  void Sphere::generateData()
  {
    auto &spheres = createChild("geometry", "geometry_spheres");
    vec3f center = vec3f{1.f, 1.f, 1.f};
    float radius = 1.f;
    spheres.createChildData("sphere.position", center);
    spheres.child("radius") = radius;
  }

  }  // namespace sg
} // namespace ospray
