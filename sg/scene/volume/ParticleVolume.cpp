// Copyright 2018 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "ParticleVolume.h"

namespace ospray {
namespace sg {

OSP_REGISTER_SG_NODE_NAME(ParticleVolume, volume_particle);

ParticleVolume::ParticleVolume() : Volume("particle")
{
  // TODO (amk): create more sensible defaults.
  createChildData("particle.position");
  createChildData("particle.radius");
  createChildData("particle.weight");
  createChild("clampMaxCumulativeValue", "float", 1.f);
  createChild("radiusSupportFactor", "float", 1.f);
  createChild("estimateValueRanges", "bool", false);
}

} // namespace sg
} // namespace ospray
