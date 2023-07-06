// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "../Material.h"

namespace ospray {
namespace sg {

struct OSPSG_INTERFACE Metal : public Material
{
  Metal();
  ~Metal() override = default;
};

OSP_REGISTER_SG_NODE_NAME(Metal, metal);

// Metal definitions //////////////////////////////////////////////////

Metal::Metal() : Material("metal")
{
  // TODO: Implement a table of common metal that pre-fill eta and k values
  // As per OSPRay documentation:
  // vec3f[]  ior  "Aluminium"
  // "data array of spectral samples of complex refractive index, each entry in
  // the form (wavelength, eta, k), ordered by wavelength (which is in nm)"
  //
  //   Metal               eta                     k
  //   Ag, Silver     (0.051, 0.043, 0.041)  (5.3, 3.6, 2.3)
  //   Al, Aluminium  (1.5, 0.98, 0.6)       (7.6, 6.6, 5.4)
  //   Au, Gold       (0.07, 0.37, 1.5)      (3.7, 2.3, 1.7)
  //   Cr, Chromium   (3.2, 3.1, 2.3)        (3.3, 3.3, 3.1)
  //   Cu, Copper     (0.1, 0.8, 1.1)        (3.5, 2.5, 2.4)

  createChild("eta",
      "rgb",
      "RGB complex refractive index, real part",
      rgb(1.5f, 0.98f, 0.6f));
  createChild("k",
      "rgb",
      "RGB complex refractive index, imaginary part",
      rgb(7.6f, 6.6f, 5.4f));
  createChild(
      "roughness", "float", "roughness in [0–1], 0 is perfect mirror", 0.1f)
      .setMinMax(0.f, 1.f);
}

} // namespace sg
} // namespace ospray
