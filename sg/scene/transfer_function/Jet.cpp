// Copyright 2009-2020 Intel Corporation
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
    std::vector<vec3f> colors;
    colors.emplace_back(0, 0, 0.562493);
    colors.emplace_back(0, 0, 1);
    colors.emplace_back(0, 1, 1);
    colors.emplace_back(0.500008, 1, 0.500008);
    colors.emplace_back(1, 1, 0);
    colors.emplace_back(1, 0, 0);
    colors.emplace_back(0.500008, 0, 0);

    createChildData("color", colors);
  }

  }  // namespace sg
} // namespace ospray
