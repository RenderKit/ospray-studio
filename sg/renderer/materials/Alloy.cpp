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
  createChild("color",
      "rgb",
      "reflectivity at normal incidence (0 degree, linear RGB)",
      rgb(0.9f));
  createChild("edgeColor",
      "rgb",
      "reflectivity at grazing angle (90 degree, linear RGB)",
      rgb(1.f));
  createChild(
      "roughness", "float", "roughness, in [0–1], 0 is perfect mirror", 0.1f)
      .setMinMax(0.f, 1.f);
}

} // namespace sg
} // namespace ospray
