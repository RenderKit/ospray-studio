// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "TransferFunction.h"

namespace ospray {
namespace sg {

// Turbo /////////////////////////////////////////////////////////////////////

struct OSPSG_INTERFACE Turbo : public TransferFunction
{
  Turbo();
  virtual ~Turbo() override = default;
};

OSP_REGISTER_SG_NODE_NAME(Turbo, transfer_function_turbo);

Turbo::Turbo() : TransferFunction("piecewiseLinear")
{
  colors.clear();
  colors.emplace_back(0.190, 0.072, 0.232);
  colors.emplace_back(0.276, 0.421, 0.891);
  colors.emplace_back(0.158, 0.736, 0.923);
  colors.emplace_back(0.197, 0.949, 0.595);
  colors.emplace_back(0.644, 0.990, 0.234);
  colors.emplace_back(0.933, 0.812, 0.227);
  colors.emplace_back(0.984, 0.493, 0.128);
  colors.emplace_back(0.816, 0.185, 0.018);
  colors.emplace_back(0.480, 0.016, 0.011);

  initOpacities();

  createChildData("color", colors);
  createChildData("opacity", opacities);
}

}
}
