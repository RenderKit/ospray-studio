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
}

NodeType Transform::type() const
{
  return NodeType::TRANSFORM;
}

OSP_REGISTER_SG_NODE_NAME(Transform, transform);

} // namespace sg
} // namespace ospray
