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
    switch (node.type()) {
    case NodeType::MATERIAL: {
      auto &mat = *node.nodeAs<Material>();
      if (mat.osprayMaterialType() == "obj") {
        if(!mat["handles"].hasChild(rendererType))
          mat["handles"].createChild(
            rendererType,
            "Node",
            cpp::Material(rendererType, mat.osprayMaterialType()));
      } else {
        if(!mat["handles"].hasChild("pathtracer"))
          mat["handles"].createChild(
              "pathtracer",
              "Node",
              cpp::Material("pathtracer", mat.osprayMaterialType()));
      }
      return false;
    }
    default:
      return true;
    }
  }

}  // namespace ospray::sg
