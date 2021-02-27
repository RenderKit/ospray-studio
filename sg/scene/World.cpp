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
  createChild("saveMetaData", "bool", false);
}

void World::preCommit() {}

void World::postCommit()
{
  if (child("saveMetaData").valueAs<bool>()) {
    auto &frame = parents().front();
    auto &fb = frame->childAs<sg::FrameBuffer>("framebuffer");
    auto &geomIdmap = fb.ge;
    auto &instanceIdmap = fb.in;
    traverse<RenderScene>(geomIdmap, instanceIdmap);
  } else
    traverse<RenderScene>();
}

OSP_REGISTER_SG_NODE_NAME(World, world);

} // namespace sg
} // namespace ospray
