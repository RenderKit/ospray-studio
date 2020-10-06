// Copyright 2020 Intel Corporation
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
  createChild("intensity", "float", "intensity of the light (a factor)", 1.f);
  createChild("color", "rgb", "color of the light", vec3f(1.f));
  createChild("type", "string", "OSPRay light type", type);

  child("intensity").setMinMax(0.f, 10.f);

  child("type").setSGOnly();
}

NodeType Light::type() const
{
  return NodeType::LIGHT;
}

} // namespace sg
} // namespace ospray
