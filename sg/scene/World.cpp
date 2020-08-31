// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "World.h"
#include "lights/Lights.h"

namespace ospray {
  namespace sg {

  World::World()
  {
    setHandle(cpp::World());
    createChild("lights", "lights");
  }

  void World::preCommit() {    
    auto ospWorld = valueAs<cpp::World>();
    auto &lights = childAs<sg::Lights>("lights");
    std::vector<cpp::Light> lightObjects;

    for (auto &name : lights.lightNames) {
      auto &l = lights.child(name).valueAs<cpp::Light>();
      lightObjects.push_back(l);
    }

    ospWorld.setParam("light", cpp::CopiedData(lightObjects));
  }

  void World::postCommit() {
    auto ospWorld = valueAs<cpp::World>();
    ospWorld.commit();
  }

  OSP_REGISTER_SG_NODE_NAME(World, world);

  }  // namespace sg
} // namespace ospray
