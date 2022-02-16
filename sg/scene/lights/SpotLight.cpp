// Copyright 2021-2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Light.h"

namespace ospray {
namespace sg {

struct OSPSG_INTERFACE SpotLight : public Light
{
  SpotLight();
  virtual ~SpotLight() override = default;

};

OSP_REGISTER_SG_NODE_NAME(SpotLight, spot);

// SpotLight definitions /////////////////////////////////////////////

SpotLight::SpotLight() : Light("spot")
{
  createChild("position",
      "vec3f",
      "center of the spotlight, in world-space",
      vec3f(0.f));
  createChild(
      "direction", "vec3f", "main emission direction", vec3f(0.f, 0.f, 1.f));
  createChild("openingAngle",
      "float",
      "full opening angle (in degree) of the spot;\n"
      "outside of this cone is no illumination",
      180.f);
  createChild("penumbraAngle",
      "float",
      "size (in degrees) of the 'penumbra',\n"
      "the region between the rim (of the illumination cone) and full intensity of the spot;\n"
      "should be smaller than half of openingAngle",
      5.f);
  createChild("radius", "float", 0.f);
  createChild("innerRadius", "float", 0.f);

  child("intensityQuantity")
      .setValue(uint8_t(OSP_INTENSITY_QUANTITY_INTENSITY));

  child("direction").setMinMax(-1.f, 1.f); // per component min/max
  child("openingAngle").setMinMax(0.f, 180.f);
  child("penumbraAngle").setMinMax(0.f, 180.f);

  // SpotLight supports a photometric measuredSource
  addMeasuredSource();
}

} // namespace sg
} // namespace ospray
