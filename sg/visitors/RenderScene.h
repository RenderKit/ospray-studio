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

#include "sg/Node.h"

namespace ospray {
  namespace sg {

    struct RenderScene : public Visitor
    {
      RenderScene() = default;

      bool operator()(Node &node, TraversalContext &ctx) override;

     private:
      // Helper Functions //

      void createGeometry(Node &node);

      // Data //

      struct
      {
        affine3f transform;
        cpp::Group group;
        // Apperance information:
        //     - Material
        //     - TransferFunction
        //     - ...others?
        cpp::Material material{nullptr};
        cpp::TransferFunction transferFunction{nullptr};
      } current;
    };

    // Inlined definitions ////////////////////////////////////////////////////

    inline bool RenderScene::operator()(Node &node, TraversalContext &ctx)
    {
      auto type = node.type();

      bool traverseChildren = false;

      switch (type) {
      case NodeType::GEOMETRY:
        createGeometry(node);
        break;
      default:
        traverseChildren = true;
        break;
      }

      return traverseChildren;
    }

    void RenderScene::createGeometry(Node &node)
    {
    }

  }  // namespace sg
}  // namespace ospray