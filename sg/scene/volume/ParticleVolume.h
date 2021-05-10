// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Volume.h"

namespace ospray {
  namespace sg {

  struct OSPSG_INTERFACE ParticleVolume : public Volume
  {
    ParticleVolume();
    virtual ~ParticleVolume() override = default;
  };

  OSP_REGISTER_SG_NODE_NAME(ParticleVolume, volume_particle);

  // UnstructuredVolume definitions /////////////////////////////////////////////

  ParticleVolume::ParticleVolume() : Volume("particle")
  {
    //AMK: initializing with empty data. Is this a problem?
    createChildData("particle.position");
    createChildData("particle.radius");
    createChildData("particle.weight");
    createChildData("clampMaxCumulativeValue");
    createChildData("radiusSupportFactor");
    createChildData("estimateValueRanges");
  }

  }  // namespace sg
} // namespace ospray
