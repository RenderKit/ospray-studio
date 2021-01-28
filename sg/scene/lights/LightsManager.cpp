// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "LightsManager.h"
#include "../../sg/renderer/Renderer.h"

namespace ospray {
  namespace sg {

    OSP_REGISTER_SG_NODE_NAME(LightsManager, lights);

    LightsManager::LightsManager()
    {}

    NodeType LightsManager::type() const
    {
      return NodeType::LIGHTS;
    }

    bool LightsManager::lightExists(std::string name)
    {
      auto found = std::find(lightNames.begin(), lightNames.end(), name);
      return (found != lightNames.end());
    }

    bool LightsManager::addLight(std::string name, std::string lightType)
    {
      if (name == "" || lightExists(name))
        return false;

      lightNames.push_back(name);
      createChild(name, lightType);
      return true;
    }

    bool LightsManager::addLight(NodePtr light)
    {
      if (lightExists(light->name()))
        return false;

      lightNames.push_back(light->name());
      add(light);
      return true;
    }

    void LightsManager::addLights(std::vector<NodePtr> &lights)
    {
      for (auto &l : lights) {
        if (lightExists(l->name()))
          continue;

        lightNames.push_back(l->name());
        add(l);
      }
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
      for (auto &name : lightNames) {
        removeLight(name);
      }
    }

    void LightsManager::preCommit()
    {
      cppLightObjects.clear();

      for (auto &name : lightNames) {
        auto &l = child(name);
        if (l.subType() == "hdri" && currentWorld) {
          auto &frame = currentWorld->parents().front();
          auto &renderer = frame->childAs<sg::Renderer>("renderer");
          renderer["backgroundColor"] = vec4f(0.f);
        }

        cppLightObjects.emplace_back(l.valueAs<cpp::Light>());
      }
    }

    void LightsManager::postCommit()
    {
    }

    void LightsManager::updateWorld(World &world)
    {
      if (lightNames.empty()) {
        lightNames.push_back("default-ambient");
        auto &l = createChild("default-ambient", "ambient");
      } else if (children().size() > 1 && hasChild("default-ambient")
          && rmDefaultLight) {
        // remove default light
        removeLight("default-ambient");
      }
      currentWorld = &world;
      // Commit lightsManager changes then apply lightObjects on the world.
      commit();

      if (!cppLightObjects.empty())
        world.handle().setParam("light", cpp::CopiedData(cppLightObjects));
      else
        world.handle().removeParam("light");

      world.handle().commit();
    }

  } // namespace sg
}  // namespace ospray
