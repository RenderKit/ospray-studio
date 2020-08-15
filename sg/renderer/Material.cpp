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

#include "Material.h"
#include "sg/texture/Texture2D.h"

namespace ospray::sg {

  Material::Material(std::string t) : matType(t)
  {
    createChild("handles");
  }

  NodeType Material::type() const
  {
    return NodeType::MATERIAL;
  }

  std::string Material::osprayMaterialType() const
  {
    return matType;
  }

  void Material::preCommit()
  {
    const auto &c       = children();
    const auto &handles = child("handles").children();

    if (c.empty() || handles.empty())
      return;
    for (auto &c_child : c)
      if (c_child.second->type() == NodeType::PARAMETER)
        if (!c_child.second->sgOnly())
          for (auto &h : handles) {
            // OSPRay SciVis renderer only uses kd and d params
            if (h.first == "scivis" &&
                !(c_child.first == "kd" || c_child.first == "d"))
              continue;
            c_child.second->setOSPRayParam(
                c_child.first, h.second->valueAs<cpp::Material>().handle());
          }
  }

  void Material::postCommit()
  {
    const auto &handles = child("handles").children();
    for (auto &h : handles)
      h.second->valueAs<cpp::Material>().commit();
  }

}  // namespace ospray::sg
