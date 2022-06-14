// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "../Material.h"

namespace ospray {
  namespace sg {

  struct OSPSG_INTERFACE MaterialOBJ : public Material
  {
    MaterialOBJ();
    ~MaterialOBJ() override = default;
  };

  OSP_REGISTER_SG_NODE_NAME(MaterialOBJ, obj);

  // MaterialOBJ definitions //////////////////////////////////////////////////

  MaterialOBJ::MaterialOBJ() : Material("obj") {
    createChild("kd", "rgb", "diffuse color", vec3f(0.8f));
    createChild("ks", "rgb", "specular color", vec3f(0.f));
    createChild("ns", "float", "shininess (Phong) [2-1e5]", 10.f)
        .setMinMax(2.f, 1e5f);
    createChild("d", "float", "opacity [0-1]", 1.f);
    createChild("tf", "rgb", "transparency filter color", vec3f(0.f));
  }

  }  // namespace sg
} // namespace ospray
