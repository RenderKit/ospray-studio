// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "../Material.h"

namespace ospray {
  namespace sg {

  struct OSPSG_INTERFACE Glass : public Material
  {
    Glass();
    ~Glass() override = default;
  };

  OSP_REGISTER_SG_NODE_NAME(Glass, glass);

  // Glass definitions //////////////////////////////////////////////////

  Glass::Glass() : Material("glass")
  {

  }

  }  // namespace sg
} // namespace ospray
