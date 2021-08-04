// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "TransferFunction.h"

namespace ospray {
namespace sg {

// Cool to warm ////////////////////////////////////////////////////////////////////

struct OSPSG_INTERFACE CoolToWarm : public TransferFunction
{
  CoolToWarm();
  virtual ~CoolToWarm() override = default;
};

OSP_REGISTER_SG_NODE_NAME(CoolToWarm, transfer_function_coolToWarm);

CoolToWarm::CoolToWarm() : TransferFunction("piecewiseLinear")
{
  colors.clear();
  colors.emplace_back(0.223, 0.299, 0.754);
  colors.emplace_back(0.407, 0.538, 0.934);
  colors.emplace_back(0.603, 0.731, 1.000);
  colors.emplace_back(0.788, 0.846, 0.939);
  colors.emplace_back(0.867, 0.867, 0.867);
  colors.emplace_back(0.931, 0.820, 0.761);
  colors.emplace_back(0.968, 0.657, 0.537);
  colors.emplace_back(0.887, 0.414, 0.325);
  colors.emplace_back(0.706, 0.016, 0.150);

  // Initialize to a simple ramp
  auto numSamples = colors.size();
  opacities.resize(numSamples);
  for (auto i = 0; i < numSamples; i++)
    opacities.at(i) = (float)i / (numSamples - 1);

  createChildData("color", colors);
  createChildData("opacity", opacities);
}

}
}
