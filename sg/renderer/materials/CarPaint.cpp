// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "../Material.h"

namespace ospray {
  namespace sg {

  struct OSPSG_INTERFACE CarPaint : public Material
  {
    CarPaint();
    ~CarPaint() override = default;
  };

  OSP_REGISTER_SG_NODE_NAME(CarPaint, carPaint);

  // CarPaint definitions //////////////////////////////////////////////////

  CarPaint::CarPaint() : Material("carPaint")
  {

  }

  }  // namespace sg
} // namespace ospray
