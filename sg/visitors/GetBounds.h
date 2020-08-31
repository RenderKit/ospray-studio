// ======================================================================== //
// Copyright 2009-2019 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

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
