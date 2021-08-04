// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "TransferFunction.h"

namespace ospray {
namespace sg {

// Viridis ///////////////////////////////////////////////////////////////////

struct OSPSG_INTERFACE Viridis : public TransferFunction
{
  Viridis();
  virtual ~Viridis() override = default;
};

OSP_REGISTER_SG_NODE_NAME(Viridis, transfer_function_viridis);

Viridis::Viridis() : TransferFunction("piecewiseLinear")
{
  colors.clear();
  colors.emplace_back(0.267, 0.005, 0.329);
  colors.emplace_back(0.279, 0.175, 0.483);
  colors.emplace_back(0.230, 0.322, 0.546);
  colors.emplace_back(0.173, 0.449, 0.558);
  colors.emplace_back(0.128, 0.567, 0.551);
  colors.emplace_back(0.158, 0.684, 0.502);
  colors.emplace_back(0.369, 0.789, 0.383);
  colors.emplace_back(0.678, 0.864, 0.190);
  colors.emplace_back(0.993, 0.906, 0.144);

  initOpacities();

  createChildData("color", colors);
  createChildData("opacity", opacities);
}

// Plasma ////////////////////////////////////////////////////////////////////

struct OSPSG_INTERFACE Plasma : public TransferFunction
{
  Plasma();
  virtual ~Plasma() override = default;
};

OSP_REGISTER_SG_NODE_NAME(Plasma, transfer_function_plasma);

Plasma::Plasma() : TransferFunction("piecewiseLinear")
{
  colors.clear();
  colors.emplace_back(0.050, 0.030, 0.528);
  colors.emplace_back(0.300, 0.010, 0.632);
  colors.emplace_back(0.495, 0.012, 0.658);
  colors.emplace_back(0.665, 0.139, 0.586);
  colors.emplace_back(0.798, 0.280, 0.470);
  colors.emplace_back(0.902, 0.425, 0.360);
  colors.emplace_back(0.973, 0.586, 0.252);
  colors.emplace_back(0.993, 0.772, 0.155);
  colors.emplace_back(0.940, 0.975, 0.131);

  initOpacities();

  createChildData("color", colors);
  createChildData("opacity", opacities);
}

// Magma /////////////////////////////////////////////////////////////////////

struct OSPSG_INTERFACE Magma : public TransferFunction
{
  Magma();
  virtual ~Magma() override = default;
};

OSP_REGISTER_SG_NODE_NAME(Magma, transfer_function_magma);

Magma::Magma() : TransferFunction("piecewiseLinear")
{
  colors.clear();
  colors.emplace_back(0.001, 0.000, 0.014);
  colors.emplace_back(0.113, 0.065, 0.277);
  colors.emplace_back(0.317, 0.072, 0.485);
  colors.emplace_back(0.513, 0.148, 0.508);
  colors.emplace_back(0.716, 0.215, 0.475);
  colors.emplace_back(0.904, 0.320, 0.388);
  colors.emplace_back(0.987, 0.536, 0.382);
  colors.emplace_back(0.997, 0.770, 0.535);
  colors.emplace_back(0.987, 0.991, 0.750);

  initOpacities();

  createChildData("color", colors);
  createChildData("opacity", opacities);
}

// Inferno ///////////////////////////////////////////////////////////////////

struct OSPSG_INTERFACE Inferno : public TransferFunction
{
  Inferno();
  virtual ~Inferno() override = default;
};

OSP_REGISTER_SG_NODE_NAME(Inferno, transfer_function_inferno);

Inferno::Inferno() : TransferFunction("piecewiseLinear")
{
  colors.clear();
  colors.emplace_back(0.001, 0.000, 0.014);
  colors.emplace_back(0.129, 0.047, 0.291);
  colors.emplace_back(0.342, 0.062, 0.429);
  colors.emplace_back(0.541, 0.135, 0.415);
  colors.emplace_back(0.736, 0.216, 0.330);
  colors.emplace_back(0.894, 0.353, 0.194);
  colors.emplace_back(0.978, 0.558, 0.035);
  colors.emplace_back(0.975, 0.798, 0.206);
  colors.emplace_back(0.988, 0.998, 0.645);

  initOpacities();

  createChildData("color", colors);
  createChildData("opacity", opacities);
}
} // namespace sg
} // namespace ospray
