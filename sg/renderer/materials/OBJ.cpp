// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "../Material.h"

namespace ospray {
  namespace sg {

  struct OSPSG_INTERFACE MaterialOBJ : public Material
  {
    MaterialOBJ();
    ~MaterialOBJ() override = default;
  };

  OSP_REGISTER_SG_NODE_NAME(MaterialOBJ, obj);

  // MaterialOBJ definitions //////////////////////////////////////////////////

  MaterialOBJ::MaterialOBJ() : Material("obj") {}

  }  // namespace sg
} // namespace ospray
