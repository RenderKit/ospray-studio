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

#include <stack>

namespace ospray {
  namespace sg {

    enum class TreeState
    {
      ALLOPEN,
      ALLCLOSED,
      ROOTOPEN
    };

    static bool g_WidgetUpdated = false;

    struct GenerateImGuiWidgets : public Visitor
    {
      GenerateImGuiWidgets(TreeState state = TreeState::ALLCLOSED,
                           bool &u         = g_WidgetUpdated);

      bool operator()(Node &node, TraversalContext &ctx) override;
      void postChildren(Node &, TraversalContext &ctx) override;

     private:
      std::stack<int> openLevels;
      TreeState initState;
      bool &updated;
    };

    // Specialized widget generators //////////////////////////////////////////

    inline bool generateWidget_bool(const std::string &title, Node &node)
    {
      bool b = node.valueAs<bool>();

      if (node.readOnly()) {
        ImGui::Text("%s",
                    (node.name() + ": " + (b ? "true" : "false")).c_str());
        return false;
      }

      if (ImGui::Checkbox(title.c_str(), &b)) {
        node.setValue(b);
        return true;
      }

      return false;
    }

   inline bool generateWidget_int(const std::string &title, Node &node)
    {
      int i = node.valueAs<int>();

      if (node.readOnly()) {
        ImGui::Text("%s", (node.name() + ": " + std::to_string(i)).c_str());
        return false;
      }

      if (node.hasMinMax()) {
        const int min = node.minAs<int>();
        const int max = node.maxAs<int>();
        if (ImGui::SliderInt(title.c_str(), &i, min, max)) {
          node.setValue(i);
          return true;
        }
      } else {
        if (ImGui::DragInt(title.c_str(), &i, 1)) {
          node.setValue(i);
          return true;
        }
      }

      return false;
    }

   inline bool generateWidget_float(const std::string &title, Node &node)
    {
      float f = node.valueAs<float>();

      if (node.readOnly()) {
        ImGui::Text("%s", (node.name() + ": " + std::to_string(f)).c_str());
        return false;
      }

      if (node.hasMinMax()) {
        const float min = node.minAs<float>();
        const float max = node.maxAs<float>();
        if (ImGui::SliderFloat(title.c_str(), &f, min, max)) {
          node.setValue(f);
          return true;
        }
      } else {
        if (ImGui::DragFloat(title.c_str(), &f, 0.01f)) {
          node.setValue(f);
          return true;
        }
      }

      return false;
    }

   inline bool generateWidget_vec2i(const std::string &title, Node &node)
    {
      vec2i v = node.valueAs<vec2i>();

      if (node.readOnly()) {
        ImGui::Text("%s",
                    (node.name() + ": " + std::to_string(v.x) + ", " +
                     std::to_string(v.y))
                        .c_str());
        return false;
      }

      if (node.hasMinMax()) {
        const int min = node.minAs<int>();
        const int max = node.maxAs<int>();
        if (ImGui::SliderInt2(title.c_str(), v, min, max)) {
          node.setValue(v);
          return true;
        }
      } else {
        if (ImGui::DragInt2(title.c_str(), v)) {
          node.setValue(v);
          return true;
        }
      }

      return false;
    }

   inline bool generateWidget_vec2f(const std::string &title, Node &node)
    {
      vec2f v = node.valueAs<vec2f>();

      if (node.readOnly()) {
        ImGui::Text("%s",
                    (node.name() + ": " + std::to_string(v.x) + ", " +
                     std::to_string(v.y))
                        .c_str());
        return false;
      }

      if (node.hasMinMax()) {
        const float min = node.minAs<float>();
        const float max = node.maxAs<float>();
        if (ImGui::SliderFloat2(title.c_str(), v, min, max)) {
          node.setValue(v);
          return true;
        }
      } else {
        if (ImGui::DragFloat2(title.c_str(), v)) {
          node.setValue(v);
          return true;
        }
      }

      return false;
    }

   inline bool generateWidget_vec3i(const std::string &title, Node &node)
    {
      vec3i v = node.valueAs<vec3i>();

      if (node.readOnly()) {
        ImGui::Text("%s",
                    (node.name() + ": " + std::to_string(v.x) + ", " +
                     std::to_string(v.y) + ", " + std::to_string(v.z))
                        .c_str());
        return false;
      }

      if (node.hasMinMax()) {
        const int min = node.minAs<int>();
        const int max = node.maxAs<int>();
        if (ImGui::SliderInt3(title.c_str(), v, min, max)) {
          node.setValue(v);
          return true;
        }
      } else {
        if (ImGui::DragInt3(title.c_str(), v)) {
          node.setValue(v);
          return true;
        }
      }

      return false;
    }

   inline bool generateWidget_vec3f(const std::string &title, Node &node)
    {
      vec3f v = node.valueAs<vec3f>();

      if (node.readOnly()) {
        ImGui::Text("%s",
                    (node.name() + ": " + std::to_string(v.x) + ", " +
                     std::to_string(v.y) + ", " + std::to_string(v.z))
                        .c_str());
        return false;
      }

      if (node.hasMinMax()) {
        const float min = node.minAs<float>();
        const float max = node.maxAs<float>();
        if (ImGui::SliderFloat3(title.c_str(), v, min, max)) {
          node.setValue(v);
          return true;
        }
      } else {
        if (ImGui::DragFloat3(title.c_str(), v)) {
          node.setValue(v);
          return true;
        }
      }

      return false;
    }

   inline bool generateWidget_rgb(const std::string &title, Node &node)
    {
      vec3f v = node.valueAs<vec3f>();

      if (node.readOnly()) {
        ImGui::Text("%s",
                    (node.name() + ": " + std::to_string(v.x) + ", " +
                     std::to_string(v.y) + ", " + std::to_string(v.z))
                        .c_str());
        return false;
      }

      if (ImGui::ColorEdit3(title.c_str(), v)) {
        node.setValue(v);
        return true;
      }

      return false;
    }

   inline bool generateWidget_rgba(const std::string &title, Node &node)
    {
      vec4f v = node.valueAs<vec4f>();

      if (node.readOnly()) {
        ImGui::Text("%s",
                    (node.name() + ": " + std::to_string(v.x) + ", " +
                     std::to_string(v.y) + ", " + std::to_string(v.z) + ", " +
                     std::to_string(v.w))
                        .c_str());
        return false;
      }

      if (ImGui::ColorEdit4(title.c_str(), v)) {
        node.setValue(v);
        return true;
      }

      return false;
    }

   inline bool generateWidget_affine3f(const std::string &title, Node &node)
    {
      affine3f a = node.valueAs<affine3f>();

      if (node.readOnly()) {
        ImGui::Text("%s", (node.name() + ": affine3f").c_str());
        return false;
      }

      ImGui::Text("%s", "linear space");
      if (ImGui::DragFloat3("l.vx", a.l.vx) ||
          ImGui::DragFloat3("l.vy", a.l.vy) ||
          ImGui::DragFloat3("l.vz", a.l.vz)) {
        node.setValue(a);
        return true;
      }

      ImGui::Text("%s", "affine space");
      if (ImGui::DragFloat3("p", a.p)) {
        node.setValue(a);
        return true;
      }

      return false;
    }

   inline bool generateWidget_string(const std::string &title, Node &node)
    {
      std::string s = node.valueAs<std::string>();

      ImGui::Text("%s", (node.name() + ": \"" + s + "\"").c_str());
      return false;
    }

    using WidgetGenerator = bool (*)(const std::string &, Node &);
    static std::map<std::string, WidgetGenerator> widgetGenerators = {
        {"bool", generateWidget_bool},
        {"int", generateWidget_int},
        {"float", generateWidget_float},
        {"vec2i", generateWidget_vec2i},
        {"vec2f", generateWidget_vec2f},
        {"vec3i", generateWidget_vec3i},
        {"vec3f", generateWidget_vec3f},
        {"rgb", generateWidget_rgb},
        {"rgba", generateWidget_rgba},
        {"affine3f", generateWidget_affine3f},
        {"string", generateWidget_string},
    };

    // Inlined definitions ////////////////////////////////////////////////////

    inline GenerateImGuiWidgets::GenerateImGuiWidgets(TreeState state, bool &u)
        : initState(state), updated(u)
    {
    }

    inline bool GenerateImGuiWidgets::operator()(Node &node,
                                                 TraversalContext &ctx)
    {
      std::string widgetName = node.name();

      auto generator = widgetGenerators[node.subType()];

      if (generator) {
        widgetName += "##" + std::to_string(node.uniqueID());
        if (generator(widgetName, node))
          updated = true;
      } else if (node.hasChildren()) {
        ImGuiTreeNodeFlags open =
            (initState == TreeState::ALLOPEN ||
             (initState == TreeState::ROOTOPEN && ctx.level == 0))
                ? ImGuiTreeNodeFlags_DefaultOpen
                : ImGuiTreeNodeFlags_None;

        if (ImGui::TreeNodeEx(widgetName.c_str(), open)) {
          openLevels.push(ctx.level);

          // this node may be strongly-typed and contain a parameter
          if (node.type() == NodeType::TRANSFORM) {
            widgetName += "##" + std::to_string(node.uniqueID());
            if (generateWidget_affine3f(widgetName, node))
              updated = true;
          }  // else if (other types) {}

        } else {
          return false; // tree closed, don't process children
        }
      } else {
        // there is no generator for this type
        widgetName += ": " + node.subType();
        ImGui::Text("%s", widgetName.c_str());
      }

      return true;
    }

    inline void GenerateImGuiWidgets::postChildren(Node &, TraversalContext &ctx)
    {
      // TreePop (unindent) only after levels that we opened
      if (openLevels.size() > 0 && ctx.level == openLevels.top()) {
        ImGui::TreePop();
        openLevels.pop();
      }
    }

  }  // namespace sg
}  // namespace ospray
