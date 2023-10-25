// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Camera.h"

namespace ospray {
namespace sg {

Camera::Camera(const std::string &type)
{
  auto handle = ospNewCamera(type.c_str());
  setHandle(handle);

  createChild("position", "vec3f", vec3f(0.f));
  createChild("direction", "vec3f", vec3f(0.0f, 0.0f, 1.f));
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

  createChild("shutterType",
      "OSPShutterType",
      "type of shutter, for motion blur\n"
      "  0 = global\n"
      "  1 = rolling right\n"
      "  2 = rolling left\n"
      "  3 = rolling down\n"
      "  4 = rolling up",
      OSP_SHUTTER_GLOBAL);
  child("shutterType")
      .setMinMax(OSP_SHUTTER_GLOBAL, OSP_SHUTTER_ROLLING_UP);

  createChild("rollingShutterDuration",
      "float",
      "for a rolling shutter (see shutterType), defines the \"open\" time per line",
      0.0f);
  child("rollingShutterDuration").setMinMax(0.f, 1.f);

  createChild("shutter", "range1f", range1f(0.0f));
  child("shutter").setSGNoUI();

  createChild("uniqueCameraName", "string", "default", std::string("default"));
  child("uniqueCameraName").setSGOnly();
  child("uniqueCameraName").setSGNoUI();

  // optional camera Index for a camera in a multi-camera scene
  // to be set by the application or importer
  createChild("cameraId",
      "int",
      "sets index of the camera in a scene with multiple cameras",
      0);
  child("cameraId").setSGOnly();
  child("cameraId").setSGNoUI();
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
