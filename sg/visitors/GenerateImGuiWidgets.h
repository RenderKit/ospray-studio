// ======================================================================== //
// Copyright 2020 Intel Corporation                                         //
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
#include "imgui.h"

namespace ospray {
  namespace sg {

    struct GenerateImGuiWidgets : public Visitor
    {
      GenerateImGuiWidgets() = default;

      bool operator()(Node &node, TraversalContext &ctx) override;
      void postChildren(Node &, TraversalContext &) override;
    };

    // Inlined definitions ////////////////////////////////////////////////////

    inline bool GenerateImGuiWidgets::operator()(Node &node,
                                                 TraversalContext &ctx)
    {
      // ALOK: used to copy and assign values from/to children.
      // if we don't, sg doesn't know the node was updated
      static float f1;
      static vec3f f3;

      if (ImGui::TreeNode(node.name().c_str())) {
        if (node.type() == NodeType::LIGHT) {
          f1 = node["intensity"].valueAs<float>();
          if (ImGui::SliderFloat("intensity", &f1, 0.f, 1.f))
            node["intensity"].setValue(f1);
          f3 = node["color"].valueAs<vec3f>();
          if (ImGui::ColorEdit3("color", f3))
            node["color"].setValue(f3);

          return false;
        }
        return true;
      } else {
        // the tree is closed, no need to continue
        return false;
      }
    }

    inline void GenerateImGuiWidgets::postChildren(Node &, TraversalContext &)
    {
      ImGui::TreePop();
    }

  }  // namespace sg
}  // namespace ospray
