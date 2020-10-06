// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Animation.h"

namespace ospray {
  namespace sg {

  Animation::Animation(){}

  NodeType Animation::type() const
  {
    return NodeType::ANIMATION;
  }

  OSP_REGISTER_SG_NODE_NAME(Animation, animation);

  }  // namespace sg
} // namespace ospray
