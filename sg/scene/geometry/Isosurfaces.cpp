// Copyright 2009 Intel Corporation
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

Isosurfaces::Isosurfaces() : Geometry("isosurface")
{
  // Don't allow isosurfaces to be set as clipping geometry.  The result doesn't
  // look correct and it may crash.
  child("isClipping").setSGNoUI();
}

} // namespace sg
} // namespace ospray

