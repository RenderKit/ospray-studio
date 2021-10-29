// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../Node.h"

namespace ospray {
namespace sg {

struct OSPSG_INTERFACE Transform : public Node_T<affine3f>
{
  Transform();
  ~Transform() override = default;

  NodeType type() const override;

  affine3f accumulatedXfm{one};

  virtual void preCommit() override;
};

} // namespace sg
} // namespace ospray
