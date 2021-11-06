// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Transform.h"
#include "sg/camera/Camera.h"

namespace ospray {
namespace sg {

Transform::Transform()
{
  createChild("translation", "vec3f", vec3f(zero));
  createChild("rotation", "quaternionf", quaternionf(one));
  createChild("scale", "vec3f", vec3f(one));
  setValue(affine3f(one));
  createChild(
      "dynamicScene", "bool", "faster BVH build, slower ray traversal", false);
  createChild("compactMode",
      "bool",
      "tell Embree to use a more compact BVH in memory by trading ray traversal performance",
      false);
  createChild("robustMode",
      "bool",
      "tell Embree to enable more robust ray intersection code paths(slightly slower)",
      false);
}

NodeType Transform::type() const
{
  return NodeType::TRANSFORM;
}

void Transform::preCommit()
{
  // TODO: if we can somehow mark camera as modified here...
  // we should move the following code to camera class
  if (hasChildOfSubType("camera_perspective")) {
    auto &cameras = childrenOfSubType("camera_perspective");
    for (auto c : cameras) {
      auto newPosition = xfmPoint(accumulatedXfm, vec3f(0));
      auto newDirection = xfmVector(accumulatedXfm, vec3f(0, 0, -1));
      auto newUp = xfmVector(accumulatedXfm, vec3f(0, 1, 0));
      c->child("position").setValue(newPosition);
      c->child("direction").setValue(newDirection);
      c->child("up").setValue(newUp);
    }

  } else if (hasChildOfSubType("camera_orthographic")) {
    auto &cameras = childrenOfSubType("camera_orthographic");
    for (auto c : cameras) {
      auto newPosition = xfmPoint(accumulatedXfm, vec3f(0));
      auto newDirection = xfmVector(accumulatedXfm, vec3f(0, 0, -1));
      auto newUp = xfmVector(accumulatedXfm, vec3f(0, 1, 0));
      c->child("position").setValue(newPosition);
      c->child("direction").setValue(newDirection);
      c->child("up").setValue(newUp);
    }
  }
}

OSP_REGISTER_SG_NODE_NAME(Transform, transform);

} // namespace sg
} // namespace ospray
