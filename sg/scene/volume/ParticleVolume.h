// Copyright 2009-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "Volume.h"

namespace ospray {
namespace sg {

struct OSPSG_INTERFACE ParticleVolume : public Volume
{
  ParticleVolume();
  virtual ~ParticleVolume() override = default;
};

} // namespace sg
} // namespace ospray
