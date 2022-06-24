// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Light.h"

namespace ospray {
namespace sg {

struct OSPSG_INTERFACE PointLight : public Light
{
  PointLight();
  virtual ~PointLight() override = default;
};

OSP_REGISTER_SG_NODE_NAME(PointLight, sphere);

// PointLight definitions /////////////////////////////////////////////

PointLight::PointLight() : Light("sphere")
{
  createChild("position",
      "vec3f",
      "center of the sphere light, in world-space",
      vec3f(0.f));
  createChild("radius", "float", "size of the sphere light", 0.f);
  createChild("direction",
      "vec3f",
      "main orientation of `intensityDistribution`",
      vec3f(0.f, 0.f, 1.f)).setMinMax(-1.f, 1.f);

  child("intensityQuantity")
      .setValue(uint8_t(OSP_INTENSITY_QUANTITY_INTENSITY));

  // PointLight supports a photometric measuredSource
  addMeasuredSource();
}

} // namespace sg
} // namespace ospray
