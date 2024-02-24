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
  createChild("reflectance",
      "rgb",
      "diffuse reflectance of the surface",
      rgb(.4f, 0.f, 0.f));
  createChild("backScattering", "float", "amount of back scattering", .5f);
  createChild("horizonScatteringColor",
      "rgb",
      "color of horizon scattering",
      rgb(.75f, .1f, .1f));
  createChild("horizonScatteringFallOff",
      "float",
      "fall-off of horizon scattering",
      10.f);
}

} // namespace sg
} // namespace ospray
