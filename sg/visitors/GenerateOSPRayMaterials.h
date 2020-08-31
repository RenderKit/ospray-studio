// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../renderer/Material.h"
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
  {
  }

  inline bool GenerateOSPRayMaterials::operator()(Node &node,
                                                  TraversalContext &)
  {
    // XXX: How can a renderer specify that it doesn't handle materials?
    if (rendererType == "debug")
      return true;

    switch (node.type()) {
    case NodeType::MATERIAL: {
      auto &mat         = *node.nodeAs<Material>();
      auto &matChildren = mat.children();
      if (rendererType == "pathtracer" &&
          !mat["handles"].hasChild("pathtracer")) {
        auto &matHandle = mat["handles"].createChild(
            "pathtracer",
            "Node",
            cpp::Material("pathtracer", mat.osprayMaterialType()));
        if (!matChildren.empty())
          for (auto &c : matChildren)
            if (c.second->subType() == "texture_2d" ||
                c.second->subType() == "texture_volume")
              c.second->setOSPRayParam(
                  c.first, matHandle.valueAs<cpp::Material>().handle());
      } else if (!mat["handles"].hasChild(rendererType) &&
                 mat.osprayMaterialType() == "obj") {
        auto &matHandle = mat.child("handles").createChild(
            rendererType,
            "Node",
            cpp::Material(rendererType, mat.osprayMaterialType()));
        if (!matChildren.empty())
          for (auto &c : matChildren)
            if (c.second->subType() == "texture_2d" ||
                c.second->subType() == "texture_volume")
              c.second->setOSPRayParam(
                  c.first, matHandle.valueAs<cpp::Material>().handle());
      }
      return false;
    }
    default:
      return true;
    }
  }

  }  // namespace sg
} // namespace ospray
