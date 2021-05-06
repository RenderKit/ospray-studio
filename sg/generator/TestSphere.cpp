// Copyright 2009-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Generator.h"
// std
#include <random>

namespace ospray {
namespace sg {

struct Sphere : public Generator
{
  Sphere();
  ~Sphere() override = default;

  void generateData() override;
};

OSP_REGISTER_SG_NODE_NAME(Sphere, generator_sphere);

// Sphere definitions ////////////////////////////////////////////////

Sphere::Sphere()
{
  auto &parameters = child("parameters");
  parameters.createChild("center", "vec3f", vec3f(1.f));
  parameters.createChild("radius", "float", 1.f);
  parameters.createChild("color", "rgba", vec4f(1.f));
  parameters.child("radius").setMinMax(.0f, 10.f);

  auto &xfm = createChild("xfm", "transform");
}

void Sphere::generateData()
{
  auto &parameters = child("parameters");
  auto center = parameters["center"].valueAs<vec3f>();
  auto radius = parameters["radius"].valueAs<float>();
  auto color = parameters["color"].valueAs<rgba>();

  auto &xfm = child("xfm");
  auto &sphere = xfm.createChild("geometry", "geometry_spheres");

  sphere.createChildData("sphere.position", center);
  sphere.child("radius") = radius;
  // color will be added to the geometric model, it is not directly part
  // of the spheres primitive
  sphere.createChild("color", "rgba", color);
  sphere.child("color").setSGOnly();
}

} // namespace sg
} // namespace ospray
