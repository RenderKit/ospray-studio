// Copyright 2009-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Geometry.h"

namespace ospray {
namespace sg {

struct OSPSG_INTERFACE Isosurfaces : public Geometry
{
  Isosurfaces();
  virtual ~Isosurfaces() override = default;
};

OSP_REGISTER_SG_NODE_NAME(Isosurfaces, geometry_isosurfaces);

// Isosurfaces definitions ////////////////////////////////////////////////

Isosurfaces::Isosurfaces() : Geometry("isosurface") {}

} // namespace sg
} // namespace ospray

