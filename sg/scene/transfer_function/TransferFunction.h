// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../../Node.h"

namespace ospray {
namespace sg {

struct OSPSG_INTERFACE TransferFunction
    : public OSPNode<cpp::TransferFunction, NodeType::TRANSFER_FUNCTION>
{
  TransferFunction(const std::string &osp_type);
  ~TransferFunction() override = default;

  std::vector<vec3f> colors;
  std::vector<float> opacities;
};

} // namespace sg
} // namespace ospray
