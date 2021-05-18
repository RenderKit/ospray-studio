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

  float clampMaxCumulativeValue;
  float radiusSupportFactor;
  bool estimateValueRanges;

  void generateData() override;
};

OSP_REGISTER_SG_NODE_NAME(ParticleVol, generator_particle_volume);

// ParticleVol definitions
// //////////////////////////////////////////////

ParticleVol::ParticleVol()
{
  auto &parameters = child("parameters");

  parameters.createChild("dimensions", "vec3i", vec3i(10));
  parameters.createChild("gridOrigin", "vec3f", vec3f(-1.f));
  parameters.createChild("gridSpacing", "vec3f", vec3f(1.f));

  auto &xfm = createChild("xfm", "transform");
}

void ParticleVol::generateData()
{
  // create random number distributions for point center and weight
  int32_t randomSeed = 0;
  std::mt19937 gen(randomSeed);

  range1f weightRange(0.5f, 1.5f);
  box3f bounds(vec3f(0.f), vec3f(10.f));

  std::uniform_real_distribution<float> centerDistribution_x(bounds.lower.x, bounds.upper.x);
  std::uniform_real_distribution<float> centerDistribution_y(bounds.lower.y, bounds.upper.y);
  std::uniform_real_distribution<float> centerDistribution_z(bounds.lower.z, bounds.upper.z);
  std::uniform_real_distribution<float> radiusDistribution(.25f, 1.0f);
  std::uniform_real_distribution<float> weightDistribution(weightRange.lower, weightRange.upper);

  size_t numParticles = 100;

  position.resize(numParticles);
  radius.resize(numParticles);
  weight.resize(numParticles);

  tasking::parallel_for(numParticles, [&](size_t i) {
    position[i] = vec3f(centerDistribution_x(gen), centerDistribution_y(gen), centerDistribution_z(gen));
    radius[i] = radiusDistribution(gen);
    weight[i] = weightDistribution(gen);
  });

  clampMaxCumulativeValue = weightRange.upper;
  radiusSupportFactor = 3.f;
  estimateValueRanges = false;

  auto &xfm = child("xfm");
  auto &tf = xfm.createChild("transferFunction", "transfer_function_jet");

  auto &pvol = tf.createChild("pvol", "volume_particle");
  pvol.createChildData("particle.position", position, true);
  pvol.createChildData("particle.radius", radius, true);
  pvol.createChildData("particle.weight", weight, true);
  pvol.createChild("clampMaxCumulativeValue", "float", clampMaxCumulativeValue);
  pvol.createChild("radiusSupportFactor", "float", radiusSupportFactor);
  pvol.createChild("estimateValueRanges", "bool", estimateValueRanges);
}

} // namespace sg
} // namespace ospray
