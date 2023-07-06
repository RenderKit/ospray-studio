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
  createChild("eta", "float", "index of refraction (practical range)", 1.5f)
      .setMinMax(1.f, 4.f);
  createChild("attenuationColor",
      "rgb",
      "resulting color due to attenuation (linear RGB)",
      rgb(1.f));
  createChild(
      "attenuationDistance", "float", "distance affecting attenuation", 1.f)
      .setMinMax(0.f, 10000.f);
  createChild("thickness", "float", "virtual thickness", 1.f)
      .setMinMax(0.f, 10000.f);
}

} // namespace sg
} // namespace ospray
