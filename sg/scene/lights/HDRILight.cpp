// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Light.h"

namespace ospray {
namespace sg {

struct OSPSG_INTERFACE HDRILight : public Light
{
  HDRILight();
  virtual ~HDRILight() override = default;
  void postCommit() override;
};

OSP_REGISTER_SG_NODE_NAME(HDRILight, hdri);

// HDRILight definitions /////////////////////////////////////////////

HDRILight::HDRILight() : Light("hdri")
{
  createChild("up",
      "vec3f",
      "up direction of the light in world-space",
      vec3f(0.f, 1.f, 0.f));
  createChild("direction",
      "vec3f",
      "direction to which the center of the texture will be mapped",
      vec3f(0.f, 0.f, 1.f));

  child("up").setMinMax(-1.f, 1.f); // per component min/max
  child("direction").setMinMax(-1.f, 1.f); // per component min/max
}

void HDRILight::postCommit()
{
  auto asLight = valueAs<cpp::Light>();
  auto &map = child("map").valueAs<cpp::Texture>();
  asLight.setParam("map", (cpp::Texture)map);
  map.commit();

  this->Light::postCommit();
}

} // namespace sg
} // namespace ospray
