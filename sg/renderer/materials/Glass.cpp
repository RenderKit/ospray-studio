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
  createChild("eta", "float", "index of refraction (practical range)", 1.5f)
      .setMinMax(1.f, 4.f);
  createChild("attenuationColor",
      "rgb",
      "resulting color due to attenuation (linear RGB)",
      rgb(1.f));
  createChild(
      "attenuationDistance", "float", "distance affecting attenuation", 1.f);
}

} // namespace sg
} // namespace ospray
