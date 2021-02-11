// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Geometry.h"

namespace ospray {
  namespace sg {

    struct OSPSG_INTERFACE Curves : public Geometry
    {
      Curves();
      virtual ~Curves() override = default;
    };

    OSP_REGISTER_SG_NODE_NAME(Curves, geometry_curves);

    // Curves definitions /////////////////////////////////////////////////////

    Curves::Curves() : Geometry("curve")
    {
      createChild("radius", "float", 1.f);
    }
  }
}
