// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Geometry.h"

namespace ospray {
namespace sg {

struct OSPSG_INTERFACE Subdivision : public Geometry
{
  Subdivision();
  virtual ~Subdivision() override = default;
};

OSP_REGISTER_SG_NODE_NAME(Subdivision, geometry_subdivision);

// Subdivision definitions //////////////////////////////////////////////////

Subdivision::Subdivision() : Geometry("subdivision") {}

} // namespace sg
} // namespace ospray

