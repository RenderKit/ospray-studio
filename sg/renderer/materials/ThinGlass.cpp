// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "../Material.h"

namespace ospray {
  namespace sg {

  struct OSPSG_INTERFACE ThinGlass : public Material
  {
    ThinGlass();
    ~ThinGlass() override = default;
  };

  OSP_REGISTER_SG_NODE_NAME(ThinGlass, thinGlass);

  // ThinGlass definitions //////////////////////////////////////////////////

  ThinGlass::ThinGlass() : Material("thinGlass")
  {

  }

  }  // namespace sg
} // namespace ospray
