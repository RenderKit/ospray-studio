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
  createChild("pigmentColor", "rgb", "", rgb(1.f));
  createChild("eta", "float", "index of refraction (practical range)", 1.4f)
      .setMinMax(1.f, 4.f);
  createChild(
      "roughness", "float", "roughness in [0–1], 0 is perfect mirror", 0.01f)
      .setMinMax(0.f, 1.f);
}

} // namespace sg
} // namespace ospray
