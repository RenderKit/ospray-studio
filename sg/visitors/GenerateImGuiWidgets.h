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
      // void postChildren(Node &, TraversalContext &) override;
    };

    // Inlined definitions ////////////////////////////////////////////////////

    inline bool GenerateImGuiWidgets::operator()(Node &node,
                                                 TraversalContext &ctx)
    {
      for (int i = 0; i < ctx.level; i++)
        ImGui::Indent();

      static float f1;
      static vec3f f3;

      if (node.subType() == "Node") {
        ImGui::Text(node.name().c_str());
      } else if (node.subType() == "float") {
        f1 = node.valueAs<float>();
        if (ImGui::SliderFloat(node.name().c_str(), &f1, 0.f, 1.f))
          node.setValue(f1);
      } else if (node.subType() == "vec3f") {
        f3 = node.valueAs<vec3f>();
        if (ImGui::SliderFloat3(node.name().c_str(), f3, -1.f, 1.f))
          node.setValue(f3);
      } else {
        ImGui::Text(node.subType().c_str());
      }

      for (int i = 0; i < ctx.level; i++)
        ImGui::Unindent();

      return true;
    }

  }  // namespace sg
}  // namespace ospray
