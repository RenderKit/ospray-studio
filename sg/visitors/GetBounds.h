// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../Node.h"

namespace ospray {
  namespace sg {

  struct GetBounds : public Visitor
  {
    GetBounds() = default;

    bool operator()(Node &node, TraversalContext &ctx) override;

    box3f bounds;
  };

  // Inlined definitions //////////////////////////////////////////////////////

  inline bool GetBounds::operator()(Node &node, TraversalContext &ctx)
  {
    switch (node.type()) {
    case NodeType::WORLD: {
      auto world = node.valueAs<cpp::World>();
      bounds     = world.getBounds<box3f>();
      return false;
    }
    case NodeType::GEOMETRY: {
      auto geom = node.valueAs<cpp::Geometry>();
      bounds    = geom.getBounds<box3f>();
      return false;
    }
    default:
      return true;
    }
  }

  }  // namespace sg
} // namespace ospray
