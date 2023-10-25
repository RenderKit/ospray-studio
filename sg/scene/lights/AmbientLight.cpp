// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Light.h"

namespace ospray {
namespace sg {

struct OSPSG_INTERFACE AmbientLight : public Light
{
  AmbientLight();
  virtual ~AmbientLight() override = default;
};

OSP_REGISTER_SG_NODE_NAME(AmbientLight, ambient);

// AmbientLight definitions /////////////////////////////////////////////

AmbientLight::AmbientLight() : Light("ambient")
{
  // Ambient should be invisible by default, not directly viewable
  child("visible").setValue(false);
  child("intensityQuantity").setValue(OSP_INTENSITY_QUANTITY_IRRADIANCE);
}

} // namespace sg
} // namespace ospray
