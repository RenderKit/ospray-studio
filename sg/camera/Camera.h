// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "sg/Node.h"

namespace ospray {
  namespace sg {

  struct OSPSG_INTERFACE Camera : public OSPNode<cpp::Camera, NodeType::CAMERA>
  {
    Camera(std::string type);
    virtual ~Camera() override = default;

    NodeType type() const override;
  };

  }  // namespace sg
} // namespace ospray
