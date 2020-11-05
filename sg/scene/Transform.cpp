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
  affine3f xfm(one);
  bool mod = false;

  if (child("scale").isModified()) {
    xfm = affine3f::scale(child("scale").valueAs<vec3f>());
    mod = true;
  }
  if (child("rotation").isModified()) {
    affine3f newxfm(affine3f::rotate(child("rotation").valueAs<quaternionf>()));
    if (mod) {
      xfm = newxfm * xfm;
    } else {
      xfm = newxfm;
      mod = true;
    }
  }
  if (child("translation").isModified()) {
    xfm.p = child("translation").valueAs<vec3f>();
    mod = true;
  }

  if (mod)
    setValue(xfm);
}

OSP_REGISTER_SG_NODE_NAME(Transform, transform);

} // namespace sg
} // namespace ospray
