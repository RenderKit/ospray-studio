// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Light.h"

namespace ospray {
namespace sg {

struct OSPSG_INTERFACE QuadLight : public Light
{
  QuadLight();
  virtual ~QuadLight() override = default;
};

OSP_REGISTER_SG_NODE_NAME(QuadLight, quad);

// QuadLight definitions /////////////////////////////////////////////

QuadLight::QuadLight() : Light("quad")
{
  createChild("position", "vec3f", vec3f(0.f));
  createChild("edge1", "vec3f", vec3f(1.f, 0.f, 0.f));
  createChild("edge2", "vec3f", vec3f(0.f, 1.f, 0.f));

  child("intensityQuantity").setValue(OSP_INTENSITY_QUANTITY_RADIANCE);

  // QuadLight supports a photometric measuredSource
  addMeasuredSource();
}

} // namespace sg
} // namespace ospray
