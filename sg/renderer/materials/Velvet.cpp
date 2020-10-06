// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "../Material.h"

namespace ospray {
  namespace sg {

  struct OSPSG_INTERFACE Velvet : public Material
  {
    Velvet();
    ~Velvet() override = default;
  };

  OSP_REGISTER_SG_NODE_NAME(Velvet, velvet);

  // Velvet definitions //////////////////////////////////////////////////

  Velvet::Velvet() : Material("velvet")
  {

  }

  }  // namespace sg
} // namespace ospray
