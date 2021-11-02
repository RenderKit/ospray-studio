// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "../Material.h"

namespace ospray {
  namespace sg {

  struct OSPSG_INTERFACE Principled : public Material
  {
    Principled();
    ~Principled() override = default;
  };

  OSP_REGISTER_SG_NODE_NAME(Principled, principled);

  // Principled definitions //////////////////////////////////////////////////

  Principled::Principled() : Material("principled")
  {
    createChild("baseColor", "rgb", "diffuse reflectivity (or metal color)",
        vec3f(0.8));
    createChild("edgeColor", "rgb", "grazing edge tint (metallic only)",
        vec3f(1.f));
    createChild("metallic", "float",
        "mix between dieletric (diffuse+specular) and metallic (specular only) [0-1]",
        0.f)
        .setMinMax(0.f, 1.f);
    createChild("diffuse", "float", "diffuse reflection weight [0-1]", 1.f).setMinMax(0.f, 1.f);
    createChild("specular", "float", "specular reflection weight [0-1]", 1.f).setMinMax(0.f, 1.f);
    createChild("ior", "float", "index of refraction", 1.f);
    createChild("transmission", "float", "light tranmissivity [0-1]", 0.f).setMinMax(0.f, 1.f);
    createChild("transmissionColor", "rgb", "attenuated color in transmission", vec3f(1.f));
    createChild("transmissionDepth", "float", "distance until transmissionColor", 1.f);
    createChild("roughness", "float", "surface roughness [0-1] (0 is smooth)", 0.f).setMinMax(0.f, 1.f);
    createChild("anisotropy", "float", "amount of specular anisotropy [0-1]", 0.f).setMinMax(0.f, 1.f);
    createChild("rotation", "float", "anisotropy rotation [0-1]", 0.f).setMinMax(0.f, 1.f);
    createChild("normal", "float", "normal map/scale for all layers [0-1]", 1.f).setMinMax(0.f, 1.f);
    createChild("thin", "bool", "whether material is thin or solid", false);
    createChild("thickness", "float", "with thin, amount of attenuation", 1.f);
    createChild("backlight", "float", "with thin, amount of diffuse transmission [0-2] (1 is 50/50 reflection/transmission)", 0.f).setMinMax(0.f, 2.f);
    createChild("coat", "float", "clearcoat layer weight [0-1]", 0.f).setMinMax(0.f, 1.f);
    createChild("coatIor", "float", "clearcoat layer index of refraction", 1.5f);
    createChild("coatColor", "rgb", "clearcoat tint", vec3f(1.f));
    createChild("coatThickness", "float", "clearcoat attenuation amount", 1.f);
    createChild("coatRoughness", "float", "clearcoat roughness [0-1] (0 is smooth)", 0.f).setMinMax(0.f, 1.f);
    createChild("sheen", "float", "sheen layer weight [0-1] (emulates velvet)", 0.f).setMinMax(0.f, 1.f);
    createChild("sheenColor", "rgb", "sheen color", vec3f(1.f));
    createChild("sheenTint", "float", "amount of tint from sheenColor to baseColor", 0.f);
    createChild("sheenRoughness", "float", "sheen roughness [0-1] (0 is smooth)", 0.2f).setMinMax(0.f, 1.f);
    createChild("opacity", "float", "alpha cut-out transparency [0-1] (0 is transparent)", 1.f).setMinMax(0.f, 1.f);
  }

  }  // namespace sg
} // namespace ospray
