// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Geometry.h"

namespace ospray {
namespace sg {

struct OSPSG_INTERFACE Curves : public Geometry
{
  Curves();
  virtual ~Curves() override = default;
};

OSP_REGISTER_SG_NODE_NAME(Curves, geometry_curves);

// Curves definitions /////////////////////////////////////////////////////

Curves::Curves() : Geometry("curve")
{
  createChildData("vertex.position_radius",
      std::vector<vec4f>(
          {vec4f(-1.f, -1.f, -1.f, 1.f), vec4f(1.f, 1.f, 1.f, 1.f)}));
  createChild("type", "uchar", (uint8_t)OSP_ROUND);
  createChild("basis", "uchar", (uint8_t)OSP_LINEAR);
}
} // namespace sg
} // namespace ospray
