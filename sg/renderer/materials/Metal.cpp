// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "../Material.h"

namespace ospray {
  namespace sg {

  struct OSPSG_INTERFACE Metal : public Material
  {
    Metal();
    ~Metal() override = default;
  };

  OSP_REGISTER_SG_NODE_NAME(Metal, metal);

  // Metal definitions //////////////////////////////////////////////////

  Metal::Metal() : Material("metal")
  {

  }

  }  // namespace sg
} // namespace ospray
