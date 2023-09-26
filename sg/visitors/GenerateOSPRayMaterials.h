// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "sg/renderer/Material.h"
#include "sg/texture/Texture2D.h"

namespace ospray {
namespace sg {

struct GenerateOSPRayMaterials : public Visitor
{
  GenerateOSPRayMaterials(std::string rendererType);
  ~GenerateOSPRayMaterials() override = default;

  bool operator()(Node &node, TraversalContext &) override;

 private:
  std::string rendererType;
};

// Inlined definitions //////////////////////////////////////////////////////

inline GenerateOSPRayMaterials::GenerateOSPRayMaterials(std::string type)
    : rendererType(type)
{}

inline bool GenerateOSPRayMaterials::operator()(Node &node, TraversalContext &)
{
  // XXX: How can a renderer specify that it doesn't handle materials?
  if (rendererType == "debug")
    return true;

  // Only process material nodes
  if (node.type() != NodeType::MATERIAL) {
    return true;
  }

  auto &mat = *node.nodeAs<Material>();

  // Only the pathtracer handles material types other than OBJ
  if (rendererType != "pathtracer" && mat.osprayMaterialType() != "obj")
    return true;

  // Create a renderer specific material node if it doesn't already exist
  if (!mat["handles"].hasChild(rendererType)) {
    auto &matHandle = mat["handles"].createChild(rendererType,
        "Node",
        cpp::Material(mat.osprayMaterialType()));
    for (auto &c : mat.children())
      if (c.second->subType() == "texture_2d"
          || c.second->subType() == "texture_volume")
        c.second->setOSPRayParam(
            c.first, matHandle.valueAs<cpp::Material>().handle());
  }

  return false;
}

} // namespace sg
} // namespace ospray
