// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Light.h"
#include "Photometric.h"

namespace ospray {
namespace sg {

struct OSPSG_INTERFACE SpotLight : public Light
{
  SpotLight();
  virtual ~SpotLight() override = default;

 protected:
  void preCommit() override;
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

  createChild("measuredSource",
      "string",
      "File containing intensityDistribution data to modulate\n"
      "the intensity per direction. (EULUMDAT format)",
      std::string(""))
      .setSGOnly();

  child("intensityQuantity")
      .setValue(uint8_t(OSP_INTENSITY_QUANTITY_INTENSITY));

  child("direction").setMinMax(-1.f, 1.f); // per component min/max
  child("openingAngle").setMinMax(0.f, 180.f);
  child("penumbraAngle").setMinMax(0.f, 180.f);
}

void SpotLight::preCommit()
{
  if (child("measuredSource").isModified()) {
    remove("intensityDistribution");
    remove("c0");
    auto fileName = child("measuredSource").valueAs<std::string>();
    if (fileName != "") {
      Eulumdat lamp(fileName);
      if (lamp.load()) {
        createChildData("intensityDistribution", lamp.lid);
        // Set c0 if iSym = 0 (asymmetric)
#if 0 // XXX Set it to what exactly?  Default is already perpendicular
        // to direction
        if (lamp.iSym == 0)
          createChild("c0", "vec3f", vec3f(0.f, 0.f, 1.f));
#endif
      }
    }
  }

  Light::preCommit();
}

} // namespace sg
} // namespace ospray
