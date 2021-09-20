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
    createChild("color", "rgb", "light emission color", vec3f(1.f));
    createChild("intensity", "float", "light brightness", 1.f);
    createChild("transparency", "float", "material transparency", 1.f);
  }

  }  // namespace sg
} // namespace ospray
