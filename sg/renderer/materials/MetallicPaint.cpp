// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "../Material.h"

namespace ospray {
namespace sg {

struct OSPSG_INTERFACE MetallicPaint : public Material
{
  MetallicPaint();
  ~MetallicPaint() override = default;
};

OSP_REGISTER_SG_NODE_NAME(MetallicPaint, metallicPaint);

// MetallicPaint definitions //////////////////////////////////////////////////

MetallicPaint::MetallicPaint() : Material("metallicPaint")
{
  createChild("baseColor", "rgb", "base color", rgb(0.8f));
  createChild("flakeColor", "rgb", "metallic flake color", rgb(0.9f));
  createChild("flakeAmount", "float", "amount of flakes [0-1]", 0.3f)
      .setMinMax(0.f, 1.f);
  createChild("flakeSpread", "float", "flake spread [0-1]", 0.35f)
      .setMinMax(0.f, 1.f);
  createChild("eta", "float", "index of refraction of clearcoat", 1.5f)
      .setMinMax(1.f, 4.f);
}

} // namespace sg
} // namespace ospray
