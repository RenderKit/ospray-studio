// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../Node.h"
#include <stack>

namespace ospray {
  namespace sg {

    struct OSPSG_INTERFACE Animation : public Node
    {
      Animation();
      virtual ~Animation() override = default;

      NodeType type() const override;

      std::stack<affine3f> xfms;
    };

  }  // namespace sg
}  // namespace ospray
