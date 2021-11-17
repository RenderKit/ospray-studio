// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "LightsManager.h"

namespace ospray {
namespace sg {

OSP_REGISTER_SG_NODE_NAME(LightsManager, lights);

LightsManager::LightsManager()
{
  addLight("default-ambient", "ambient");
}

NodeType LightsManager::type() const
{
  return NodeType::LIGHTS;
}

bool LightsManager::lightExists(std::string name)
{
  auto found = std::find(lightNames.begin(), lightNames.end(), name);
  return (found != lightNames.end());
}

// Add a light node (main entry)
bool LightsManager::addLight(NodePtr light)
{
  if (lightExists(light->name()))
    return false;

  // remove default light
  if (hasChild("default-ambient") && rmDefaultLight)
    removeLight("default-ambient");

  // When adding HDRI or sunSky, set background color to black.
  // It's otherwise confusing.  The user can still adjust it afterward.
  if (light->subType() == "hdri" || light->subType() == "sunSky") {
    if (!parents().empty()) {
      auto &frame = parents().front();
      auto &renderer = frame->childAs<sg::Renderer>("renderer");
      renderer["backgroundColor"] = rgba(vec3f(0.f), 1.f); // black, opaque
    }
  }

  lightNames.push_back(light->name());
  add(light);
  return true;
}

// Add a vector of lights nodes at once (helper)
void LightsManager::addLights(std::vector<NodePtr> &lights)
{
  for (auto &l : lights)
    addLight(l);
}

// Add light by name and type (helper)
bool LightsManager::addLight(std::string name, std::string lightType)
{
  return name == "" ? false : addLight(createNode(name, lightType));
}

bool LightsManager::removeLight(std::string name)
{
  if (name == "" || !lightExists(name))
    return false;

  remove(name);
  auto found = std::find(lightNames.begin(), lightNames.end(), name);
  lightNames.erase(found);

  return true;
}

void LightsManager::clear()
{
  // removeLight modifies the lightNames vector, make a copy.
  auto tempNames = lightNames;
  for (auto &name : tempNames) {
    removeLight(name);
  }

  // Re-add the default-ambient light and gray background, when clearing lights
  addLight("default-ambient", "ambient");
  auto &frame = parents().front();
  auto &renderer = frame->childAs<sg::Renderer>("renderer");
  renderer["backgroundColor"] = rgba(vec3f(0.1f), 1.f); // Near black
}

void LightsManager::preCommit()
{
  cppLightObjects.clear();

  for (auto &name : lightNames)
    cppLightObjects.emplace_back(child(name).valueAs<cpp::Light>());
}

void LightsManager::postCommit()
{
  auto &frame = parents().front();
  auto &world = frame->childAs<sg::World>("world");

  if (!lightsInstanced)
    updateWorld(world);
}

// On a change of world or lightsManager, set the new lights list on the world
void LightsManager::updateWorld(World &world)
{
  if (!cppLightObjects.empty())
    world.handle().setParam("light", cpp::CopiedData(cppLightObjects));
  else
    world.handle().removeParam("light");

  world.handle().commit();
}

} // namespace sg
} // namespace ospray
