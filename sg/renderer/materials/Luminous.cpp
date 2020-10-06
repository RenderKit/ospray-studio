// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "../Material.h"

namespace ospray {
  namespace sg {

  struct OSPSG_INTERFACE Luminous : public Material
  {
    Luminous();
    ~Luminous() override = default;
  };

  OSP_REGISTER_SG_NODE_NAME(Luminous, luminous);

  // Luminous definitions //////////////////////////////////////////////////

  Luminous::Luminous() : Material("luminous")
  {

  }

  }  // namespace sg
} // namespace ospray
