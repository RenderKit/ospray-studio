// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Geometry.h"

namespace ospray {
  namespace sg {

  struct OSPSG_INTERFACE Boxes : public Geometry
  {
    Boxes();
    virtual ~Boxes() override = default;
  };

  OSP_REGISTER_SG_NODE_NAME(Boxes, geometry_boxes);

  // Boxes definitions //////////////////////////////////////////////////////

  Boxes::Boxes() : Geometry("box")
  {
    createChildData("box", box3f({vec3f(-1.f,-1.f,-1.f),vec3f(1.f,1.f,1.f)}));
  }

  }  // namespace sg
} // namespace ospray
