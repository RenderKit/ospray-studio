// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "sg/Node.h"
#include "app/ArcballCamera.h"

namespace ospray {
  namespace sg {

  struct OSPSG_INTERFACE Camera : public OSPNode<cpp::Camera, NodeType::CAMERA>
  {
    Camera(const std::string &type);
    virtual ~Camera() override = default;

    NodeType type() const override;

    void setState(std::shared_ptr<CameraState> _cs) {
      cs = _cs;
    };

    std::shared_ptr<CameraState> getState() {
      return cs;
    };

    bool animate{false};

   private:
    std::shared_ptr<CameraState> cs;
  };

  }  // namespace sg
} // namespace ospray
