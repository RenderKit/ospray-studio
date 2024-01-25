// Copyright 2009 Intel corporation
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
  void preCommit() override;
  void postCommit() override;

  NodeType type() const override;

  OSPPixelFilterType pixelFilter{OSP_PIXELFILTER_GAUSS};
};

} // namespace sg
} // namespace ospray
