// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Geometry.h"

namespace ospray {
  namespace sg {

  struct OSPSG_INTERFACE Triangles : public Geometry
  {
    Triangles();
    virtual ~Triangles() override = default;
  };

  OSP_REGISTER_SG_NODE_NAME(Triangles, geometry_triangles);

  // Triangles definitions ////////////////////////////////////////////////////

  Triangles::Triangles() : Geometry("mesh") {}

  }  // namespace sg
} // namespace ospray
