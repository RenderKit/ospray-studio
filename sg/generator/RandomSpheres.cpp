// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Generator.h"
#include "sg/Mpi.h"

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
  std::vector<vec3f> normals;
  std::vector<vec2f> texCoords;
  std::vector<vec4f> colors;
};

OSP_REGISTER_SG_NODE_NAME(RandomSpheres, generator_random_spheres);

// RandomSpheres definitions ////////////////////////////////////////////////

RandomSpheres::RandomSpheres()
{
  auto &parameters = child("parameters");
  parameters.createChild("numSpheres", "int", (int)1e6);
  parameters.createChild("radius", "float", .002f);
  parameters.createChild("type", "OSPSphereType", OSP_SPHERE);
  parameters.child("numSpheres").setMinMax(1, (int)10e6);
  parameters.child("radius").setMinMax(.001f, .1f);
	parameters.child("type").setMinMax(OSP_SPHERE, OSP_ORIENTED_DISC);
  parameters.createChild("generateColors", "bool", true);

  auto &xfm = createChild("xfm", "transform");
}

void RandomSpheres::generateData()
{
  auto &parameters = child("parameters");
  auto &type = parameters["type"].valueAs<OSPSphereType>();
  auto &numSpheres = parameters["numSpheres"].valueAs<int>();
  auto &radius = parameters["radius"].valueAs<float>();
  auto &generateColors = parameters["generateColors"].valueAs<bool>();

  auto &xfm = child("xfm");
  auto &spheres = xfm.createChild("spheres", "geometry_spheres");

  // Distribute centers within a unit cube
  std::mt19937 rng(0);
  std::uniform_real_distribution<float> rgb(0.f, 1.f);

  centers.resize(numSpheres);
  normals.resize(numSpheres);
  texCoords.resize(numSpheres);
  colors.resize(numSpheres);

  std::uniform_real_distribution<float> dist_x, dist_y, dist_z;

  if (sgUsingMpi())
  {
    //divide up world space by number of MPI ranks and set centers according to local rank
    const vec3i grid = compute_grid(sgMpiWorldSize());
    const vec3i brickId(sgMpiRank() % grid.x, (sgMpiRank() / grid.x) % grid.y, sgMpiRank() / (grid.x * grid.y));
    vec3f brick_dims = vec3f(1.f) / grid;

    box3f brick;
    brick.lower = brickId * brick_dims;
    brick.upper = brickId * brick_dims + brick_dims;

    const float radEps = radius + 1e-6f;

    dist_x = std::uniform_real_distribution<float>(brick.lower.x + radEps, brick.upper.x - radEps);
    dist_y = std::uniform_real_distribution<float>(brick.lower.y + radEps, brick.upper.y - radEps);
    dist_z = std::uniform_real_distribution<float>(brick.lower.z + radEps, brick.upper.z - radEps);

    for (int i = 0; i < numSpheres; ++i) {
      centers[i] = vec3f(dist_x(rng), dist_y(rng), dist_z(rng));
      normals[i] = normalize(vec3f(0.f) - centers[i]);
      texCoords[i] = vec2f((centers[i].x + 1) * 0.5f , (centers[i].y + 1) * 0.5f);
      colors[i] = vec4f(float(sgMpiRank() % sgMpiWorldSize()), 1.f, float((sgMpiRank() + 1) % sgMpiWorldSize()), 1.f);
    }
  }
  else
  {
    dist_x = std::uniform_real_distribution<float>(-1.f + radius, 1.f - radius);
    dist_y = std::uniform_real_distribution<float>(-1.f + radius, 1.f - radius);
    dist_z = std::uniform_real_distribution<float>(-1.f + radius, 1.f - radius);

    for (int i = 0; i < numSpheres; ++i) {
      centers[i] = vec3f(dist_x(rng), dist_y(rng), dist_z(rng));
      normals[i] = normalize(vec3f(0.f) - centers[i]);
      texCoords[i] = vec2f((centers[i].x + 1) * 0.5f , (centers[i].y + 1) * 0.5f);
      colors[i] = vec4f(rgb(rng), rgb(rng), rgb(rng), 1.f);
    }
  }

  spheres.child("type") = type;
  spheres.createChildData("sphere.position", centers, true);
  spheres.child("radius") = radius;

	if (type == OSP_ORIENTED_DISC)
    spheres.createChildData("sphere.normal", normals, true);

  if (generateColors) {
    spheres.createChildData("color", colors, true);
    // color will be added to the geometric model, it is not directly part
    // of the spheres primitive
    spheres.child("color").setSGOnly();
  }

	// One texCoord per sphere (constant per sphere)
	spheres.createChildData("sphere.texcoord", texCoords, true);

  spheres.createChild("material", "uint32_t", uint32_t(0)); // This is a scenegraph parameter
	spheres.child("material").setReadOnly();
  spheres.child("material").setSGOnly();
}

} // namespace sg
} // namespace ospray
