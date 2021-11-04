// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "../Material.h"

namespace ospray {
  namespace sg {

  struct OSPSG_INTERFACE CarPaint : public Material
  {
    CarPaint();
    ~CarPaint() override = default;
  };

  OSP_REGISTER_SG_NODE_NAME(CarPaint, carPaint);

  // CarPaint definitions //////////////////////////////////////////////////

  CarPaint::CarPaint() : Material("carPaint")
  {
    createChild("baseColor", "rgb", "base color", vec3f(0.8));
    createChild("roughness", "float", "roughness [0-1]", 0.f).setMinMax(0.f, 1.f);
    createChild("flakeColor", "rgb", "metallic flake color", vec3f(0.9));
    createChild("flakeDensity", "float", "amount of flake coverage [0-1]", 0.f)
        .setMinMax(0.f, 1.f);
    createChild("flakeScale", "float", "size of flakes", 100.f);
    createChild("flakeSpread", "float", "flake spread [0-1]", 0.3f)
        .setMinMax(0.f, 1.f);
    createChild("flakeJitter", "float", "flake randomness [0-1]", 0.75f)
        .setMinMax(0.f, 1.f);
    createChild("flakeRoughness", "float", "flake roughness [0-1]", 0.3f)
        .setMinMax(0.f, 1.f);
    createChild("coat", "float", "clearcoat layer weight [0-1]", 1.f)
        .setMinMax(0.f, 1.f);
    createChild("coatIor", "float", "index of refraction for clearcoat", 1.5f);
    createChild("coatColor", "rgb", "clearcoat color", vec3f(1.f));
    createChild("coatThickness",
        "float",
        "clearcoat layer thickness (affects attenuation)",
        1.f);
    createChild("coatRoughness", "float", "clearcoat roughness [0-1]", 0.f)
        .setMinMax(0.f, 1.f);
    createChild("flipflopColor",
        "rgb",
        "reflectivity of flakes at grazing angle (pearlescent color)",
        vec3f(1.f));
    createChild(
        "flipflopFalloff", "float", "flip flop amount [0-1] (1 disables)", 1.f)
        .setMinMax(0.f, 1.f);
  }

  }  // namespace sg
} // namespace ospray
