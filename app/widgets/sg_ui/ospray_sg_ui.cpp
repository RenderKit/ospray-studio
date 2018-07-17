// ======================================================================== //
// Copyright 2009-2018 Intel Corporation                                    //
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

#include "ospray_sg_ui.h"

#include "imgui.h"

#include "transferFunction.h"

#include <unordered_map>

namespace ospray {

  // Functions to make ImGui widgets for SG nodes /////////////////////////////

  static void sgWidget_box3f(const std::string &text,
                             std::shared_ptr<sg::Node> node)
  {
    box3f val = node->valueAs<box3f>();
    ImGui::Text("(%f, %f, %f)", val.lower.x, val.lower.y, val.lower.z);
    ImGui::SameLine();
    ImGui::Text("(%f, %f, %f)", val.upper.x, val.upper.y, val.upper.z);
  }

  static void sgWidget_vec4f(const std::string &text,
                             std::shared_ptr<sg::Node> node)
  {
    vec4f val = node->valueAs<vec4f>();
    auto nodeFlags = node->flags();
    if (nodeFlags & sg::NodeFlags::gui_readonly) {
      ImGui::Text("(%f, %f, %f, %f)", val.x, val.y, val.z, val.w);
    } else if (nodeFlags & sg::NodeFlags::gui_slider) {
      if (ImGui::SliderFloat4(text.c_str(), &val.x,
                              node->min().get<vec4f>().x,
                              node->max().get<vec4f>().x))
        node->setValue(val);
    } else if (ImGui::DragFloat4(text.c_str(), (float*)&val.x, .01f)) {
      node->setValue(val);
    }
  }

  static void sgWidget_vec3f(const std::string &text,
                             std::shared_ptr<sg::Node> node)
  {
    vec3f val = node->valueAs<vec3f>();
    auto nodeFlags = node->flags();
    if (nodeFlags & sg::NodeFlags::gui_readonly) {
      ImGui::Text("(%f, %f, %f)", val.x, val.y, val.z);
    } else if (nodeFlags & sg::NodeFlags::gui_color) {
      if (ImGui::ColorEdit3(text.c_str(), (float*)&val.x))
        node->setValue(val);
    } else if (nodeFlags & sg::NodeFlags::gui_slider) {
      if (ImGui::SliderFloat3(text.c_str(), &val.x,
                              node->min().get<vec3f>().x,
                              node->max().get<vec3f>().x))
        node->setValue(val);
    } else if (ImGui::DragFloat3(text.c_str(), (float*)&val.x, .01f)) {
      node->setValue(val);
    }
  }

  static void sgWidget_vec3i(const std::string &text,
                             std::shared_ptr<sg::Node> node)
  {
    vec3i val = node->valueAs<vec3i>();
    ImGui::Text("(%i, %i, %i)", val.x, val.y, val.z);
  }

  static void sgWidget_vec2f(const std::string &text,
                             std::shared_ptr<sg::Node> node)
  {
    vec2f val = node->valueAs<vec2f>();
    auto nodeFlags = node->flags();
    if (nodeFlags & sg::NodeFlags::gui_readonly) {
      ImGui::Text("(%f, %f)", val.x, val.y);
    } else if (ImGui::DragFloat2(text.c_str(), (float*)&val.x, .01f)) {
      node->setValue(val);
    }
  }

  static void sgWidget_vec2i(const std::string &text,
                             std::shared_ptr<sg::Node> node)
  {
    vec2i val = node->valueAs<vec2i>();
    auto nodeFlags = node->flags();
    if (nodeFlags & sg::NodeFlags::gui_readonly) {
      ImGui::Text("(%i, %i)", val.x, val.y);
    } else if (ImGui::DragInt2(text.c_str(), (int*)&val.x)) {
      node->setValue(val);
    }
  }

  static void sgWidget_float(const std::string &text,
                             std::shared_ptr<sg::Node> node)
  {
    float val = node->valueAs<float>();
    auto nodeFlags = node->flags();
    if (nodeFlags & sg::NodeFlags::gui_readonly) {
      ImGui::Text("%f", val);
    } else if ((node->flags() & sg::NodeFlags::gui_slider)) {
      if (ImGui::SliderFloat(text.c_str(), &val,
                             node->min().get<float>(),
                             node->max().get<float>()))
        node->setValue(val);
    } else if (node->flags() & sg::NodeFlags::valid_min_max) {
      if (ImGui::DragFloat(text.c_str(), &val, .01f, node->min().get<float>(), node->max().get<float>()))
        node->setValue(val);
    } else if (ImGui::DragFloat(text.c_str(), &val, .01f)) {
      node->setValue(val);
    }
  }

  static void sgWidget_bool(const std::string &text,
                            std::shared_ptr<sg::Node> node)
  {
    bool val = node->valueAs<bool>();
    auto nodeFlags = node->flags();
    if (nodeFlags & sg::NodeFlags::gui_readonly) {
      ImGui::Text(val ? "true" : "false");
    } else if (ImGui::Checkbox(text.c_str(), &val)) {
      node->setValue(val);
    }
  }

  static void sgWidget_int(const std::string &text,
                           std::shared_ptr<sg::Node> node)
  {
    int val = node->valueAs<int>();
    auto nodeFlags = node->flags();
    if (nodeFlags & sg::NodeFlags::gui_readonly) {
      ImGui::Text("%i", val);
    } else if ((node->flags() & sg::NodeFlags::gui_slider)) {
      if (ImGui::SliderInt(text.c_str(), &val,
                           node->min().get<int>(),
                           node->max().get<int>()))
        node->setValue(val);
    } else if (node->flags() & sg::NodeFlags::valid_min_max) {
      if (ImGui::DragInt(text.c_str(), &val, .01f, node->min().get<int>(), node->max().get<int>()))
        node->setValue(val);
    } else if (ImGui::DragInt(text.c_str(), &val)) {
      node->setValue(val);
    }
  }

  static void sgWidget_string(const std::string &text,
                              std::shared_ptr<sg::Node> node)
  {
    auto value = node->valueAs<std::string>();
    auto nodeFlags = node->flags();
    if (nodeFlags & sg::NodeFlags::gui_readonly) {
      ImGui::Text(value.c_str());
    } else {
      auto whitelist = node->whitelist();
      if (!whitelist.empty()) {
        int val = -1;

        std::string list;
        for (auto it = whitelist.begin(); it != whitelist.end(); ++it) {
            auto option = *it;
            if (option.get<std::string>() == value)
                val = std::distance(whitelist.begin(), it);
            list += option.get<std::string>();
            list.push_back('\0');
        }

        // add to whitelist if not found
        if (val == -1) {
          val = whitelist.size();
          whitelist.push_back(value);
          node->setWhiteList(whitelist);
          list += value;
          list.push_back('\0');
        }

        ImGui::Combo(text.c_str(), &val, list.c_str(), whitelist.size());
        node->setValue(whitelist[val]);
      } else {
        std::vector<char> buf(value.size() + 1 + 256);
        strcpy(buf.data(), value.c_str());
        buf[value.size()] = '\0';
        if (ImGui::InputText(text.c_str(), buf.data(),
                             value.size()+256,
                             ImGuiInputTextFlags_EnterReturnsTrue)) {
            node->setValue(std::string(buf.data()));
        }
      }
    }
  }

  static void sgWidget_TransferFunction(
    const std::string &text,
    std::shared_ptr<sg::Node> node)
  {
    if (!node->hasChild("transferFunctionWidget")) {
      std::shared_ptr<sg::TransferFunction> tfn =
        std::dynamic_pointer_cast<sg::TransferFunction>(node);

      node->createChildWithValue("transferFunctionWidget","Node",
                                 TransferFunction(tfn));
    }

    auto &tfnWidget =
      node->child("transferFunctionWidget").valueAs<TransferFunction>();

    static bool show_editor = true;
    ImGui::Checkbox("show_editor", &show_editor);

    if (show_editor) {
      tfnWidget.render();
      tfnWidget.drawUi();
    }
  }

  using ParameterWidgetBuilder =
      void(*)(const std::string &, std::shared_ptr<sg::Node>);

  static std::unordered_map<std::string, ParameterWidgetBuilder>
      widgetBuilders =
      {
        {"float", sgWidget_float},
        {"int", sgWidget_int},
        {"vec2i", sgWidget_vec2i},
        {"vec2f", sgWidget_vec2f},
        {"vec3f", sgWidget_vec3f},
        {"vec3i", sgWidget_vec3i},
        {"vec4f", sgWidget_vec4f},
        {"box3f", sgWidget_box3f},
        {"string", sgWidget_string},
        {"bool", sgWidget_bool},
        {"TransferFunction", sgWidget_TransferFunction}
      };

  /////////////////////////////////////////////////////////////////////////////

  void guiSGSingleNode(const std::string &baseText,
                       std::shared_ptr<sg::Node> node)
  {
    std::string text = baseText;

    auto fcn = widgetBuilders[node->type()];

    if (fcn) {
      ImGui::Text(text.c_str());
      ImGui::SameLine();
      text = "##" + std::to_string(node->uniqueID());

      fcn(text, node);
    } else if (!node->hasChildren()) {
      text += node->type();
      ImGui::Text(text.c_str());
    }
  }

} // namespace ospray
