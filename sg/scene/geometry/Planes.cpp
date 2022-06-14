// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Geometry.h"

namespace ospray {
namespace sg {

struct OSPSG_INTERFACE Planes : public Geometry
{
  Planes();
  virtual ~Planes() override = default;
};

OSP_REGISTER_SG_NODE_NAME(Planes, geometry_planes);

// Planes definitions ////////////////////////////////////////////////////

Planes::Planes() : Geometry("plane") {}

} // namespace sg
} // namespace ospray
