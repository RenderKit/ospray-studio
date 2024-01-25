// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Light.h"
#include "sg/FileWatcher.h"
#include "sg/texture/Texture2D.h"

namespace ospray {
namespace sg {

struct OSPSG_INTERFACE HDRILight : public Light
{
  HDRILight();
  virtual ~HDRILight() override = default;
  void preCommit() override;
  void postCommit() override;

 private:
  std::shared_ptr<Texture2D> defaultMapPtr;
};

OSP_REGISTER_SG_NODE_NAME(HDRILight, hdri);

static std::weak_ptr<ospray::sg::Texture2D> staticDefaultMap;

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

  child("intensityQuantity") = OSP_INTENSITY_QUANTITY_SCALE;
  child("intensityQuantity").setReadOnly();

  child("up").setMinMax(-1.f, 1.f); // per component min/max
  child("direction").setMinMax(-1.f, 1.f); // per component min/max

  // Create the default map, only once
  if (staticDefaultMap.lock()) {
    defaultMapPtr = staticDefaultMap.lock();
  } else {
    // clang-format off
    static uint32_t checker[] = {
      0x7f007f00, 0x7f007f00, 0x7f007f00, 0x7f007f00, 
      0x007f007f, 0x007f007f, 0x007f007f, 0x007f007f,
      0x7f007f00, 0x7f007f00, 0x7f007f00, 0x7f007f00, 
      0x007f007f, 0x007f007f, 0x007f007f, 0x007f007f,
      0x7f007f00, 0x7f007f00, 0x7f007f00, 0x7f007f00, 
      0x007f007f, 0x007f007f, 0x007f007f, 0x007f007f,
      0x7f007f00, 0x7f007f00, 0x7f007f00, 0x7f007f00, 
      0x007f007f, 0x007f007f, 0x007f007f, 0x007f007f};
    // clang-format on
    defaultMapPtr = createNodeAs<Texture2D>("map", "texture_2d");
    staticDefaultMap = defaultMapPtr;
    Texture2D &defaultMap = *defaultMapPtr;
    defaultMap.params.size = vec2ul(16, 8);
    defaultMap.params.components = 1;
    defaultMap.params.depth = 1;
    // preferLinear = false creates an L8 texture, rather than R8
    if (!defaultMap.load("_defaultHDRI", false, true, 4, checker))
      std::cerr << "!!!! Default HDRI texture failed!" << std::endl;
    // Set filename to read-only to prevent user removing it via UI
    defaultMap["filename"].setReadOnly();
  }
}

void HDRILight::preCommit()
{
  // Finish generic Light precommit
  Light::preCommit();

  // Mark this filename to be watched for asynchronous modification
  // (added here, because the filename node can be added or removed elsewhere)
  addFileWatcher(child("filename"), "HDRI", true);

  auto filename = child("filename").valueAs<std::string>();

  // std::cout << "HDRILight::preCommit(): " << filename << std::endl;

  // defaultMap is a fallback if there is no other map loaded
  defaultMapPtr = staticDefaultMap.lock();
  Texture2D &defaultMap = *defaultMapPtr;

  // Load HDRI file and create texture
  if (filename == "") {
    add(defaultMap);
  } else {
    auto mapFilename =
        hasChild("map") ? child("map")["filename"].valueAs<std::string>() : "";

    // reload or remove if HDRI filename changes or file has been modified
    if (child("filename").isModified() || (filename != mapFilename)) {
      // If map has changed, update HDRI filename
      if (hasChild("map") && child("map").isModified()) {
        child("filename") = mapFilename;
        // Remove texture if had a map, but mapFilename has been removed
        if (mapFilename == "")
          add(defaultMap);
      } else {
        // Otherwise, create/replace map and load HDRI filename
        auto &hdriTex = createChild("map", "texture_2d");
        auto texture = hdriTex.nodeAs<sg::Texture2D>();
        // Force reload rather than using texture cache
        texture->params.reload = (filename == mapFilename);
        if (!texture->load(filename, false, false)) {
          add(defaultMap);
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
  }

  Light::postCommit();
}

} // namespace sg
} // namespace ospray
