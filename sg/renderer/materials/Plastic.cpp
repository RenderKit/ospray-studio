// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "../Material.h"

namespace ospray {
  namespace sg {

  struct OSPSG_INTERFACE Plastic : public Material
  {
    Plastic();
    ~Plastic() override = default;
  };

  OSP_REGISTER_SG_NODE_NAME(Plastic, plastic);

  // Plastic definitions //////////////////////////////////////////////////

  Plastic::Plastic() : Material("plastic")
  {

  }

  }  // namespace sg
} // namespace ospray
