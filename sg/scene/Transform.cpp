// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Transform.h"

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

OSP_REGISTER_SG_NODE_NAME(Transform, transform);

} // namespace sg
} // namespace ospray
