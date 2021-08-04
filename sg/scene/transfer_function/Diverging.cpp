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

  initOpacities();

  createChildData("color", colors);
  createChildData("opacity", opacities);
}

// Ice fire ////////////////////////////////////////////////////////////////////////

struct OSPSG_INTERFACE IceFire : public TransferFunction
{
  IceFire();
  virtual ~IceFire() override = default;
};

OSP_REGISTER_SG_NODE_NAME(IceFire, transfer_function_iceFire);

IceFire::IceFire() : TransferFunction("piecewiseLinear")
{
  colors.clear();
  colors.emplace_back(0.000, 0.000, 0.000);
  colors.emplace_back(0.000, 0.120, 0.303);
  colors.emplace_back(0.000, 0.217, 0.525);
  colors.emplace_back(0.055, 0.345, 0.659);
  colors.emplace_back(0.128, 0.493, 0.720);
  colors.emplace_back(0.189, 0.641, 0.792);
  colors.emplace_back(0.328, 0.785, 0.873);
  colors.emplace_back(0.608, 0.892, 0.936);
  colors.emplace_back(0.881, 0.912, 0.818);
  colors.emplace_back(0.951, 0.836, 0.449);
  colors.emplace_back(0.904, 0.690, 0.000);
  colors.emplace_back(0.854, 0.511, 0.000);
  colors.emplace_back(0.777, 0.330, 0.001);
  colors.emplace_back(0.673, 0.139, 0.003);
  colors.emplace_back(0.509, 0.000, 0.000);
  colors.emplace_back(0.299, 0.000, 0.000);
  colors.emplace_back(0.016, 0.003, 0.000);

  initOpacities();

  createChildData("color", colors);
  createChildData("opacity", opacities);
}

}
}
