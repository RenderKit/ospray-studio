// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "../Material.h"

namespace ospray {
  namespace sg {

  struct OSPSG_INTERFACE Alloy : public Material
  {
    Alloy();
    ~Alloy() override = default;
  };

  OSP_REGISTER_SG_NODE_NAME(Alloy, alloy);

  // Alloy definitions //////////////////////////////////////////////////

  Alloy::Alloy() : Material("alloy")
  {

  }

  }  // namespace sg
} // namespace ospray
