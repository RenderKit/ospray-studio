// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "../Material.h"

namespace ospray {
  namespace sg {

  struct OSPSG_INTERFACE Mix : public Material
  {
    Mix();
    ~Mix() override = default;
  };

  OSP_REGISTER_SG_NODE_NAME(Mix, mix);

  // Mix definitions //////////////////////////////////////////////////

  Mix::Mix() : Material("mix")
  {
    // XXX Mix needs pointers to two existing materials and a blend factor
  }

  }  // namespace sg
} // namespace ospray
