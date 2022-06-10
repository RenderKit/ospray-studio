// Copyright 2009-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Generator.h"
// std
#include <sg/scene/volume/ParticleVolume.h>

#include "rkcommon/tasking/parallel_for.h"

#include <random>

namespace ospray {
namespace sg {

struct ParticleVol : public Generator
{
  ParticleVol();
  ~ParticleVol() override = default;

  std::vector<vec3f> position;
  std::vector<float> radius;
  std::vector<float> weight;

  void generateData() override;
};

OSP_REGISTER_SG_NODE_NAME(ParticleVol, generator_particle_volume);

// ParticleVol definitions
// //////////////////////////////////////////////

ParticleVol::ParticleVol()
{
  auto &parameters = child("parameters");

  parameters.createChild("dimensions", "vec3i", "grid size", vec3i(10));
  parameters.createChild("weightRange", "range1f", range1f(0.5f, 1.5f));
  parameters.createChild("numParticles", "int", 100);

  parameters.createChild(
      "radiusSupportFactor", "float", "particle radius multiplier", 3.f);
  parameters.createChild("estimateValueRanges",
      "bool",
      "heuristic estimation of value ranges",
      true);

  parameters["weightRange"].setMinMax(0.f, 10.f);
  parameters["numParticles"].setMinMax(1, 10000);

  auto &xfm = createChild("xfm", "transform");
}

void ParticleVol::generateData()
{
  // create random number distributions for point center and weight
  int32_t randomSeed = 0;
  std::mt19937 gen(randomSeed);

  auto &parameters = child("parameters");

  auto numParticles = parameters["numParticles"].valueAs<int>();
  auto radiusSupportFactor = parameters["radiusSupportFactor"].valueAs<float>();
  auto estimateValueRanges = parameters["estimateValueRanges"].valueAs<bool>();

  auto weightRange = parameters["weightRange"].valueAs<range1f>();
  auto dimensions = parameters["dimensions"].valueAs<vec3i>();

  box3f bounds(vec3f(0.f), dimensions);

  std::uniform_real_distribution<float> centerDistribution_x(
      bounds.lower.x, bounds.upper.x);
  std::uniform_real_distribution<float> centerDistribution_y(
      bounds.lower.y, bounds.upper.y);
  std::uniform_real_distribution<float> centerDistribution_z(
      bounds.lower.z, bounds.upper.z);
  std::uniform_real_distribution<float> radiusDistribution(.25f, 1.0f);
  std::uniform_real_distribution<float> weightDistribution(
      weightRange.lower, weightRange.upper);

  // Less than 3 particle is interfering with the VKL intervalResolutionHint
  numParticles = std::max(3, numParticles);

  position.resize(numParticles);
  radius.resize(numParticles);
  weight.resize(numParticles);

  tasking::parallel_for(numParticles, [&](int i) {
    position[i] = vec3f(centerDistribution_x(gen),
        centerDistribution_y(gen),
        centerDistribution_z(gen));
    radius[i] = radiusDistribution(gen);
    weight[i] = weightDistribution(gen);
  });

  auto &xfm = child("xfm");
  auto &tf = xfm.createChild("transferFunction", "transfer_function_turbo");
  tf["valueRange"] = vec2f(0.f, weightRange.upper);

  auto &pvol = tf.createChild("particle_volume", "volume_particle");
  pvol["valueRange"] = range1f(0.f, weightRange.upper);

  pvol.createChild("gridOrigin", "vec3f", vec3f(-1.f));
  pvol.createChild("gridSpacing", "vec3f", 1.f / dimensions);

  pvol.createChildData("particle.position", position, true);
  pvol.createChildData("particle.radius", radius, true);
  pvol.createChildData("particle.weight", weight, true);
  pvol.createChild("clampMaxCumulativeValue", "float", weightRange.upper);
  pvol["clampMaxCumulativeValue"].setMinMax(0.f, weightRange.upper);
  pvol.createChild("radiusSupportFactor", "float", radiusSupportFactor);
  pvol["radiusSupportFactor"].setMinMax(0.1f, 6.f);
  pvol.createChild("estimateValueRanges", "bool", estimateValueRanges);
  pvol.createChild("maxIteratorDepth", "int", 6);
}

} // namespace sg
} // namespace ospray
