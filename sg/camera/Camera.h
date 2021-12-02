// Copyright 2009-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "app/ArcballCamera.h"
#include "sg/Node.h"

namespace ospray {
namespace sg {

struct OSPSG_INTERFACE Camera : public OSPNode<cpp::Camera, NodeType::CAMERA>
{
  Camera(const std::string &type);
  virtual ~Camera() override = default;

  NodeType type() const override;

  virtual void preCommit() override;

  // cameraToWorld is set for scene cameras(for eg: GLTF cameras) only in RenderScene
  affine3f cameraToWorld{one};
};

} // namespace sg
} // namespace ospray
