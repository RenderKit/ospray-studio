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
    std::cout << "ParticleVolume::ParticleVolume()" << std::endl;

    //AMK: initializing with empty data. This will get overwritten by the actual importer that constructs a ParticleVolume node.
    //TODO: create more sensible defaults.
    
    createChildData("particle.position");
    createChildData("particle.radius");
    createChildData("particle.weight");
    createChildData("clampMaxCumulativeValue", 1.f);
    createChildData("radiusSupportFactor", 1.f);
    createChildData("estimateValueRanges", false);
  }

  }  // namespace sg
} // namespace ospray
