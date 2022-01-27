// Copyright 2021-2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Light.h"
#include "Photometric.h"

namespace ospray {
namespace sg {

Light::Light(std::string type)
{
  auto handle = ospNewLight(type.c_str());
  setHandle(handle);
  createChild(
      "visible", "bool", "whether the light can be seen directly", true);
  createChild("color", "rgb", "color of the light", vec3f(1.f));
  createChild("intensity", "float", "intensity of the light (a factor)", 1.f);

  // Default unknown here because light type determines default
  createChild("intensityQuantity",
      "uchar",
      "type of the value represented by a light's `intensity` parameter\n"
      "  0 = radiance (W/sr/m^2)\n"
      "  1 = irradiance (W/m^2)\n"
      "  2 = intensity (W/sr)\n"
      "  3 = power (W)\n"
      "  4 = scale, linear scaling factor for light sources with a built-in quantity\n"
      "      (e.g., `HDRI`, or `sunSky`, or when using `intensityDistribution`)",
      uint8_t(OSP_INTENSITY_QUANTITY_UNKNOWN));

  createChild("type", "string", "OSPRay light type", type).setReadOnly();

  child("intensity").setMinMax(0.f, 10.f);
  child("intensityQuantity")
      .setMinMax(uint8_t(OSP_INTENSITY_QUANTITY_RADIANCE),
          uint8_t(OSP_INTENSITY_QUANTITY_SCALE));

  child("type").setSGOnly();
}

NodeType Light::type() const
{
  return NodeType::LIGHT;
}

void Light::addMeasuredSource(std::string fileName)
{
  createChild("measuredSource",
      "string",
      "File containing intensityDistribution data to modulate\n"
      "the intensity per direction. (EULUMDAT format)",
      fileName)
      .setSGOnly();
}

void Light::preCommit()
{
  // If the light has an attached measuredSource file load it and set the
  // intensityDistribution.
  if (hasChild("measuredSource") && child("measuredSource").isModified()) {
    remove("intensityDistribution");
    remove("c0");
    auto fileName = child("measuredSource").valueAs<std::string>();
    std::ifstream f(fileName);
    if (f.good()) {
      Eulumdat lamp(fileName);
      if (lamp.load()) {
        // When using intensityDistribution, SCALE is the only supported
        // quantity
        child("intensityQuantity") = uint8_t(OSP_INTENSITY_QUANTITY_SCALE);
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

  // Finish node preCommit
  OSPNode::preCommit();
}

} // namespace sg
} // namespace ospray
