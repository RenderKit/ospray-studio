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
  setValue(affine3f(one)); // XXX will always be overwritten at first commit
}

NodeType Transform::type() const
{
  return NodeType::TRANSFORM;
}

void Transform::preCommit()
{
  // XXX cannot know which param is modified, i.e.
  // child("scale").isModified() is (always) false, despite is was
  // markAsModified()...

  affine3f xfm = affine3f::rotate(child("rotation").valueAs<quaternionf>())
      * affine3f::scale(child("scale").valueAs<vec3f>());
  xfm.p = child("translation").valueAs<vec3f>();
  setValue(xfm);
}

OSP_REGISTER_SG_NODE_NAME(Transform, transform);

} // namespace sg
} // namespace ospray
