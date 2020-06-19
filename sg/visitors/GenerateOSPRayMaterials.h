// ======================================================================== //
// Copyright 2009-2019 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#pragma once

#include "../renderer/Material.h"
#include "sg/texture/Texture2D.h"

namespace ospray::sg {

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

}  // namespace ospray::sg
