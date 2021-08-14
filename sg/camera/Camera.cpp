// Copyright 2009-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Camera.h"

namespace ospray {
  namespace sg {

  Camera::Camera(const std::string &type)
  {
    auto handle = ospNewCamera(type.c_str());
    setHandle(handle);

    createChild("position", "vec3f", vec3f(0.f));
    createChild("direction", "vec3f", vec3f(1.f));
    createChild("up", "vec3f", vec3f(0.f, 1.f, 0.f));

    createChild("nearClip", "float", 0.f);

    createChild("imageStart", "vec2f", vec2f(0.f));
    createChild("imageEnd", "vec2f", vec2f(1.f));

    createChild("lookAt", "vec3f", vec3f(1.f));
    child("lookAt").setSGOnly();

    createChild("shutter", "range1f", range1f(0.0f, 1.f));
  }

  NodeType Camera::type() const
  {
    return NodeType::CAMERA;
  }
  }  // namespace sg
} // namespace ospray
