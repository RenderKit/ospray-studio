// Copyright 2021-2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Light.h"
#include "sg/texture/Texture2D.h"

namespace ospray {
namespace sg {

struct OSPSG_INTERFACE HDRILight : public Light
{
  HDRILight();
  virtual ~HDRILight() override = default;
  void preCommit() override;
  void postCommit() override;
};

OSP_REGISTER_SG_NODE_NAME(HDRILight, hdri);

// HDRILight definitions /////////////////////////////////////////////

HDRILight::HDRILight() : Light("hdri")
{
  createChild("filename", "filename", "HDRI filename", std::string(""));
  child("filename").setSGOnly();

  createChild("up",
      "vec3f",
      "up direction of the light in world-space",
      vec3f(0.f, 1.f, 0.f));
  createChild("direction",
      "vec3f",
      "direction to which the center of the texture will be mapped",
      vec3f(0.f, 0.f, 1.f));

  child("intensityQuantity") = uint8_t(OSP_INTENSITY_QUANTITY_SCALE);
  child("intensityQuantity").setReadOnly();

  child("up").setMinMax(-1.f, 1.f); // per component min/max
  child("direction").setMinMax(-1.f, 1.f); // per component min/max
}

void HDRILight::preCommit()
{
  Light::preCommit();

  auto filename = child("filename").valueAs<std::string>();

  // Load HDRI file and create texture
  if (filename == "") {
    remove("map");
  } else {
    auto mapFilename =
        hasChild("map") ? child("map")["filename"].valueAs<std::string>() : "";
    // reload or remove if HDRI filename changes
    if (filename != mapFilename) {
      // Remove texture if had a map, but mapFilename has been removed
      if (hasChild("map") && mapFilename == "") {
        remove("map");
        child("filename") = std::string("");
      } else {
        auto &hdriTex = createChild("map", "texture_2d");
        auto texture = hdriTex.nodeAs<sg::Texture2D>();
        if (!texture->load(filename, false, false)) {
          remove("map");
          child("filename") = std::string("");
        }
      }
    }
  }
}

void HDRILight::postCommit()
{
  auto asLight = valueAs<cpp::Light>();

  if (hasChild("map")) {
    auto &map = child("map").valueAs<cpp::Texture>();
    asLight.setParam("map", (cpp::Texture)map);
    map.commit();
  } else
    asLight.removeParam("map");

  Light::postCommit();
}

} // namespace sg
} // namespace ospray
