// Copyright 2009-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "TransferFunction.h"

namespace ospray {
namespace sg {

struct OSPSG_INTERFACE Jet : public TransferFunction
{
  Jet();
  virtual ~Jet() override = default;
};

OSP_REGISTER_SG_NODE_NAME(Jet, transfer_function_jet);

// Jet definitions //////////////////////////////////////////////////////////

Jet::Jet() : TransferFunction("piecewiseLinear")
{
  colors.clear();
  colors.emplace_back(0, 0, 0.562493);
  colors.emplace_back(0, 0, 1);
  colors.emplace_back(0, 1, 1);
  colors.emplace_back(0.500008, 1, 0.500008);
  colors.emplace_back(1, 1, 0);
  colors.emplace_back(1, 0, 0);
  colors.emplace_back(0.500008, 0, 0);

  // Initialize to a simple ramp
  auto numSamples = colors.size();
  opacities.resize(numSamples);
  for (auto i = 0; i < numSamples; i++)
    opacities.at(i) = (float)i / (numSamples - 1);

  createChildData("color", colors);
  createChildData("opacity", opacities);
}

} // namespace sg
} // namespace ospray
