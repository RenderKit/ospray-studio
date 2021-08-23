// Copyright 2009-2021 Intel Corporation
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

 protected:
  inline void initOpacities()
  {
    // Initialize to a simple ramp
    auto numSamples = colors.size();
    opacities.resize(numSamples);
    for (auto i = 0; i < numSamples; i++)
      opacities.at(i) = (float)i / (numSamples - 1);
  }
};

} // namespace sg
} // namespace ospray
