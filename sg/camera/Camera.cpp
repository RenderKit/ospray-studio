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

  createChild("motion blur",
      "float",
      "camera shutter duration, affecting amount of motion blur",
      0.0f);
  child("motion blur").setSGOnly();
  child("motion blur").setMinMax(0.f, 1.f);

  createChild("shutter", "range1f", range1f(0.0f));
  child("shutter").setSGNoUI();
}

NodeType Camera::type() const
{
  return NodeType::CAMERA;
}

void Camera::preCommit()
{
  auto cameraMotionBlur = child("motion blur").valueAs<float>();
  cameraMotionBlur = std::max(0.f, std::min(1.f, cameraMotionBlur));
  child("shutter") = range1f(0.f, cameraMotionBlur);

  // call baseClass preCommit to finish node
  OSPNode::preCommit();
}

} // namespace sg
} // namespace ospray
