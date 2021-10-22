// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Light.h"

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
      "  3 = power (W)",
      uint8_t(OSP_INTENSITY_QUANTITY_UNKNOWN));

  createChild("type", "string", "OSPRay light type", type);

  child("intensity").setMinMax(0.f, 10.f);
  child("intensityQuantity")
      .setMinMax(uint8_t(OSP_INTENSITY_QUANTITY_RADIANCE),
          uint8_t(OSP_INTENSITY_QUANTITY_POWER));

  child("type").setSGOnly();
}

NodeType Light::type() const
{
  return NodeType::LIGHT;
}

// called once upon setting initial orientation of lights with transforms
void Light::initOrientation(
    std::unordered_map<std::string, std::pair<vec3f, bool>> &propMap)
{
  if (setOrientation) {
    for (auto &m : propMap) {
      auto &prop = child(m.first);
      prop = m.second.first;
      if (m.second.second)
        child(m.first).rmReadOnly();
    }
    setOrientation = false;
  }
}

} // namespace sg
} // namespace ospray
