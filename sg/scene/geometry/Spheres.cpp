// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Geometry.h"

namespace ospray {
  namespace sg {

  struct OSPSG_INTERFACE Spheres : public Geometry
  {
    Spheres();
    virtual ~Spheres() override = default;
  };

  OSP_REGISTER_SG_NODE_NAME(Spheres, geometry_spheres);

  // Spheres definitions //////////////////////////////////////////////////////

  Spheres::Spheres() : Geometry("sphere")
  {
    createChild("radius", "float", 1.f);
  }

  }  // namespace sg
} // namespace ospray
