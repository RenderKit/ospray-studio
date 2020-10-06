// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "../Material.h"

namespace ospray {
  namespace sg {

  struct OSPSG_INTERFACE Principled : public Material
  {
    Principled();
    ~Principled() override = default;
  };

  OSP_REGISTER_SG_NODE_NAME(Principled, principled);

  // Principled definitions //////////////////////////////////////////////////

  Principled::Principled() : Material("principled")
  {

  }

  }  // namespace sg
} // namespace ospray
