// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Lights.h"

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

    bool Lights::removeLight(std::string name)
    {
      if (name == "" || !lightExists(name))
        return false;

      remove(name);
      auto found = std::find(lightNames.begin(), lightNames.end(), name);
      lightNames.erase(found);

      std::cout << "Lights::removeLight " << std::endl;

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
        auto &l = child(name).valueAs<cpp::Light>();
        cppLightObjects.emplace_back(l);
      }
    }

    void Lights::postCommit()
    {
    }

    void Lights::updateWorld(World &world)
    {
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
