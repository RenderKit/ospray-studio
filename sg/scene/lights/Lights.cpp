// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Lights.h"
#include "../../sg/renderer/Renderer.h"

namespace ospray {
  namespace sg {

    OSP_REGISTER_SG_NODE_NAME(Lights, lights);

    Lights::Lights()
    {
      lightNames.push_back("ambient");
      createChild("ambient", "ambient");
    }

    NodeType Lights::type() const
    {
      return NodeType::LIGHTS;
    }

    bool Lights::lightExists(std::string name)
    {
      auto found = std::find(lightNames.begin(), lightNames.end(), name);
      return (found != lightNames.end());
    }

    bool Lights::addLight(std::string name, std::string lightType)
    {
      if (name == "" || lightExists(name))
        return false;

      lightNames.push_back(name);
      createChild(name, lightType);
      return true;
    }

    bool Lights::addLight(NodePtr light)
    {
      if (lightExists(light->name()))
        return false;

      lightNames.push_back(light->name());
      add(light);
      return true;
    }

    void Lights::addLights(std::vector<NodePtr> &lights)
    {
      for (auto &l : lights) {
        if (lightExists(l->name()))
          continue;

        lightNames.push_back(l->name());
        add(l);
      }
    }

    bool Lights::removeLight(std::string name)
    {
      if (name == "" || !lightExists(name))
        return false;

      remove(name);
      auto found = std::find(lightNames.begin(), lightNames.end(), name);
      lightNames.erase(found);

      // XXX FIX: this markAsModified is not doing anything ?!?!
      markAsModified();
      isStubborn = true; // XXX HACK, until we get markAsModified working here
      return true;
    }

    void Lights::preCommit()
    {
      isStubborn = false;
      cppLightObjects.clear();

      for (auto &name : lightNames) {
        auto &l = child(name);
        if (l.subType() == "hdri") {
          auto &frame = currentWorld->parents().front();
          auto &renderer = frame->childAs<sg::Renderer>("renderer");
          renderer["backgroundColor"] = vec4f(0.f);
        }

        cppLightObjects.emplace_back(l.valueAs<cpp::Light>());
      }
    }

    void Lights::postCommit()
    {
    }

    void Lights::updateWorld(World &world)
    {
      currentWorld = &world;
      // Commit lightsManager changes then apply lightObjects on the world.
      commit();

      if (!cppLightObjects.empty())
        world.handle().setParam("light", cpp::CopiedData(cppLightObjects));
      else
        world.handle().removeParam("light");

      world.handle().commit();
    }

  }  // namespace sg
}  // namespace ospray
