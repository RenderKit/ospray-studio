// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Geometry.h"

namespace ospray {
namespace sg {

struct OSPSG_INTERFACE Isosurfaces : public Geometry
{
  Isosurfaces();
  virtual ~Isosurfaces() override = default;

#if 0 // Are these useful?  Or just create nodes?
  std::vector<float> isovalues;
  std::vector<vec3f> colors;
  cpp::Volume ospVolume;
#endif
};

OSP_REGISTER_SG_NODE_NAME(Isosurfaces, geometry_isosurfaces);

// Isosurfaces definitions ////////////////////////////////////////////////

Isosurfaces::Isosurfaces() : Geometry("isosurface") {}

// XXX preCommit/postCommit???
//    if (node.hasChild("isovalue"))
//      model.setParam("isovalue", node["volume"].valueAs<cpp::CopiedData>());

} // namespace sg
} // namespace ospray

