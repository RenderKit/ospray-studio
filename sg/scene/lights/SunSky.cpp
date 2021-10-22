// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Light.h"

namespace ospray {
namespace sg {

struct OSPSG_INTERFACE SunSky : public Light
{
  SunSky();
  virtual ~SunSky() override = default;

 protected:
  void preCommit() override;
};

OSP_REGISTER_SG_NODE_NAME(SunSky, sunSky);

// Sunsky definitions //////////////////////////////////////////////////////

SunSky::SunSky() : Light("sunSky")
{
  createChild(
      "up", "vec3f", "zenith of sky in world-space", vec3f(0.f, 1.f, 0.f));
  createChild("right",
      "vec3f",
      "right-pointing vector, such that up cross right = direction",
      vec3f(0.f, 0.f, 1.f));
  createChild("azimuth", "float", "sun's angular distance from North", 0.f);
  createChild(
      "elevation", "float", "sun's angle relative to the horizon", 90.f);
  createChild("albedo", "float", "ground reflectance [0-1]", 0.18f);
  createChild("turbidity",
      "float",
      "atmospheric turbidity due to particles [1-10]",
      3.f);
  createChild("direction",
      "vec3f",
      "main emission direction of the sun",
      vec3f(0.f, 1.f, 0.f));
  createChild("horizonExtension",
      "float",
      "extend sky dome by stretching the horizon,\n"
      "fraction of the lower hemisphere to cover [0-1]",
      0.01f);

  child("intensityQuantity").setValue((uint8_t)OSP_INTENSITY_QUANTITY_RADIANCE);

  // Set reasonable limits, this will set slider range
  child("albedo").setMinMax(0.f, 1.f);
  child("azimuth").setMinMax(-180.f, 180.f);
  child("elevation").setMinMax(-90.f, 90.f);
  child("turbidity").setMinMax(0.f, 10.f);
  child("horizonExtension").setMinMax(0.f, 1.f);

  child("direction").setReadOnly();

  // SceneGraph internal
  child("azimuth").setSGOnly();
  child("elevation").setSGOnly();
  child("right").setSGOnly();
}

void SunSky::preCommit()
{
  if (child("direction").readOnly()) {
    const float azimuth = child("azimuth").valueAs<float>() * M_PI / 180.f;
    const float elevation = child("elevation").valueAs<float>() * M_PI / 180.f;

    vec3f up = child("up").valueAs<vec3f>();
    vec3f dir = child("right").valueAs<vec3f>();
    vec3f p1 = cross(up, dir);
    LinearSpace3f r1 = LinearSpace3f::rotate(dir, -elevation);
    vec3f p2 = r1 * p1;
    LinearSpace3f r2 = LinearSpace3f::rotate(up, azimuth);
    vec3f p3 = r2 * p2;
    vec3f direction = p3;
    if (!(std::isnan(direction.x) || std::isnan(direction.y)
            || std::isnan(direction.z))) {
      // this overwrites the "direction" child parameters, making that UI
      // element not directly useable...
      auto &directionNode = child("direction");
      directionNode.setValue(direction);
    }
  }
  Light::preCommit();
}

} // namespace sg
} // namespace ospray
