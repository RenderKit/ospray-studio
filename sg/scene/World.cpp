// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "World.h"
#include "../visitors/RenderScene.h"

namespace ospray {
namespace sg {

World::World()
{
  setHandle(cpp::World());
}

void World::preCommit()
{
}

void World::postCommit()
{
  traverse<RenderScene>();
}

OSP_REGISTER_SG_NODE_NAME(World, world);

} // namespace sg
} // namespace ospray
