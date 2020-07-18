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

#include "Light.h"

namespace ospray::sg {

  Light::Light(std::string type)
  {
    auto handle = ospNewLight(type.c_str());
    setHandle(handle);
    createChild("visible", "bool", true);
    createChild("intensity", "float", 1.f);
    createChild("color", "rgb", vec3f(1.f));
    createChild("type", "string", type);
  }

  NodeType Light::type() const
  {
    return NodeType::LIGHT;
  }

  void Light::postCommit() {
    auto &lights = valueAs<cpp::Light>();
    lights.setParam("intensity", child("intensity").valueAs<float>());
    lights.setParam("color", child("color").valueAs<vec3f>());
    lights.commit();
  }

}  // namespace ospray::sg
