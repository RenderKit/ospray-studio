// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Light.h"

namespace ospray {
  namespace sg {

  Light::Light(std::string type)
  {
    auto handle = ospNewLight(type.c_str());
    setHandle(handle);
    createChild("visible", "bool", true);
    createChild("intensity", "float", 1.f);
    createChild("color", "rgb", vec3f(1.f));
    createChild("type", "string", type);

    child("type").setSGOnly();
  }

  NodeType Light::type() const
  {
    return NodeType::LIGHT;
  }

  }  // namespace sg
} // namespace ospray
