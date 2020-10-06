// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "sg/Node.h"

namespace ospray {
  namespace sg {

  struct OSPSG_INTERFACE Renderer
      : public OSPNode<cpp::Renderer, NodeType::RENDERER>
  {
    Renderer(std::string type);
    virtual ~Renderer() override = default;

    NodeType type() const override;

    void setNavMode(bool navMode);

    OSPPixelFilterTypes pixelFilter{OSP_PIXELFILTER_GAUSS};
  };

  }  // namespace sg
} // namespace ospray
