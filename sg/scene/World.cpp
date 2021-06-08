// Copyright 2009-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "World.h"
#include "sg/fb/FrameBuffer.h"
#include "sg/visitors/RenderScene.h"

namespace ospray {
namespace sg {

World::World()
{
  setHandle(cpp::World());
  createChild(
      "dynamicScene", "bool", "faster BVH build, slower ray traversal", false);
  createChild("compactMode",
      "bool",
      "tell Embree to use a more compact BVH in memory by trading ray traversal performance",
      false);
  createChild("robustMode",
      "bool",
      "tell Embree to enable more robust ray intersection code paths(slightly slower)",
      false);
}

void World::preCommit()
{
  for (auto &c : children())
    if (c.second->type() == NodeType::PARAMETER)
      c.second->setOSPRayParam(c.first, valueAs<cpp::World>().handle());
}

void World::postCommit()
{
  traverse<RenderScene>();
}

OSP_REGISTER_SG_NODE_NAME(World, world);

} // namespace sg
} // namespace ospray
