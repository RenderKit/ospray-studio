// Copyright 2009-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Generator.h"
// std
#include <random>

namespace ospray {
namespace sg {

struct RandomSpheres : public Generator
{
  RandomSpheres();
  ~RandomSpheres() override = default;

  void generateData() override;

 private:
  std::vector<vec3f> centers;
  std::vector<vec4f> colors;
};

OSP_REGISTER_SG_NODE_NAME(RandomSpheres, generator_random_spheres);

// RandomSpheres definitions ////////////////////////////////////////////////

RandomSpheres::RandomSpheres()
{
  auto &parameters = child("parameters");
  parameters.createChild("numSpheres", "int", (int)1e6);
  parameters.createChild("radius", "float", .002f);
  parameters.child("numSpheres").setMinMax(1, (int)10e6);
  parameters.child("radius").setMinMax(.001f, .1f);
  parameters.createChild("generateColors", "bool", false);

  auto &xfm = createChild("xfm", "transform");
}

void RandomSpheres::generateData()
{
  auto &parameters = child("parameters");
  auto numSpheres = parameters["numSpheres"].valueAs<int>();
  auto radius = parameters["radius"].valueAs<float>();
  auto generateColors = parameters["generateColors"].valueAs<bool>();

  auto &xfm = child("xfm");
  auto &spheres = xfm.createChild("spheres", "geometry_spheres");

  // Distribute centers within a unit cube
  std::mt19937 rng(0);
  std::uniform_real_distribution<float> dist(-1.f + radius, 1.f - radius);
  std::uniform_real_distribution<float> rgb(0.f, 1.f);

  centers.resize(numSpheres);
  colors.resize(numSpheres);

  for (int i = 0; i < numSpheres; ++i) {
    centers[i] = vec3f(dist(rng), dist(rng), dist(rng));
    colors[i] = vec4f(rgb(rng), rgb(rng), rgb(rng), 1.f);
  }

  spheres.createChildData("sphere.position", centers, true);
  spheres.child("radius") = radius;

  if (generateColors) {
    spheres.createChildData("color", colors, true);
    // color will be added to the geometric model, it is not directly part
    // of the spheres primitive
    spheres.child("color").setSGOnly();
  }

  const std::vector<uint32_t> mID = {0};
  spheres.createChildData("material", mID); // This is a scenegraph parameter
  spheres.child("material").setSGOnly();
}

} // namespace sg
} // namespace ospray
