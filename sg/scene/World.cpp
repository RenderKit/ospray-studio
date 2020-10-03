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

void World::preCommit()
{
  auto &lights = childAs<sg::Lights>("lights");
  if (lights.isModified()) {
    lightObjects.clear();

    for (auto &name : lights.lightNames) {
      auto &l = lights.child(name).valueAs<cpp::Light>();
      lightObjects.emplace_back(l);
    }
  }
}

void World::postCommit()
{
  handle().setParam("light", cpp::CopiedData(lightObjects));
  handle().commit();
}

OSP_REGISTER_SG_NODE_NAME(World, world);

} // namespace sg
} // namespace ospray
