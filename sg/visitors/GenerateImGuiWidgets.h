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
      static bool b;
      static float f1;
      static vec3f f3;

      if (ImGui::TreeNode(node.name().c_str())) {
        if (node.type() == NodeType::LIGHT) {
          b = node["visible"].valueAs<bool>();
          if (ImGui::Checkbox("visible", &b))
            node["visible"].setValue(b);

          f1 = node["intensity"].valueAs<float>();
          if (ImGui::DragFloat("intensity", &f1, 0.1f, 0.0f, 0.0f, "%.1f"))
            node["intensity"].setValue(f1);

          f3 = node["color"].valueAs<vec3f>();
          if (ImGui::ColorEdit3("color", f3))
            node["color"].setValue(f3);

          std::string lightType = node["type"].valueAs<std::string>();
          if (lightType == "distant") {
            f3 = node["direction"].valueAs<vec3f>();
            if (ImGui::SliderFloat3("direction", f3, -1.f, 1.f))
              node["direction"].setValue(f3);

            f1 = node["angularDiameter"].valueAs<float>();
            if (ImGui::SliderFloat("angular diameter", &f1, 0.f, 1.f))
              node["angularDiameter"].setValue(f1);

          } else if (lightType == "sphere") {
            f3 = node["position"].valueAs<vec3f>();
            if (ImGui::DragFloat3("position", f3, 0.1f))
              node["position"].setValue(f3);

            f1 = node["radius"].valueAs<float>();
            if (ImGui::DragFloat("radius", &f1, 0.1f, 0.f, 0.f, "%.1f"))
              node["radius"].setValue(f1);

          } else if (lightType == "spot") {
            f3 = node["position"].valueAs<vec3f>();
            if (ImGui::DragFloat3("position", f3, 0.1f))
              node["position"].setValue(f3);

            f3 = node["direction"].valueAs<vec3f>();
            if (ImGui::SliderFloat3("direction", f3, -1.f, 1.f))
              node["direction"].setValue(f3);

            f1 = node["openingAngle"].valueAs<float>();
            if (ImGui::SliderFloat(
                    "opening angle", &f1, 0.f, 180.f, "%.0f", 1.f))
              node["openingAngle"].setValue(f1);

            f1 = node["penumbraAngle"].valueAs<float>();
            if (ImGui::SliderFloat(
                    "penumbra angle", &f1, 0.f, 180.f, "%.0f", 1.f))
              node["penumbraAngle"].setValue(f1);

            f1 = node["radius"].valueAs<float>();
            if (ImGui::DragFloat("radius", &f1, 0.1f, 0.f, 0.f, "%.1f"))
              node["radius"].setValue(f1);

          } else if (lightType == "quad") {
            f3 = node["position"].valueAs<vec3f>();
            if (ImGui::DragFloat3("position", f3, 0.1f))
              node["position"].setValue(f3);

            f3 = node["edge1"].valueAs<vec3f>();
            if (ImGui::DragFloat3("edge 1", f3, 0.1f))
              node["edge1"].setValue(f3);

            f3 = node["edge2"].valueAs<vec3f>();
            if (ImGui::DragFloat3("edge 2", f3, 0.1f))
              node["edge2"].setValue(f3);
          }

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
