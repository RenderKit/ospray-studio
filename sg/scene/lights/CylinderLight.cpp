// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Light.h"

namespace ospray {
namespace sg {

struct OSPSG_INTERFACE CylinderLight : public Light
{
  CylinderLight();
  virtual ~CylinderLight() override = default;
};

OSP_REGISTER_SG_NODE_NAME(CylinderLight, cylinder);

// CylinderLight definitions /////////////////////////////////////////////

CylinderLight::CylinderLight() : Light("cylinder")
{
  createChild("position0",
      "vec3f",
      "position of the start of the cylinder",
      vec3f(0.f));
  createChild("position1",
      "vec3f",
      "position of the end of the cylinder",
      vec3f(0.f, 0.f, 1.f));
  createChild("radius", "float", "radius of the cylinder", 1.f);

  child("intensityQuantity").setValue(OSP_INTENSITY_QUANTITY_RADIANCE);
}

} // namespace sg
} // namespace ospray
