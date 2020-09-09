// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Material.h"
#include "sg/texture/Texture2D.h"

namespace ospray {
  namespace sg {

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
            // OSPRay AO renderer only uses kd and d params
            if (h.first == "ao" &&
                !(c_child.first == "kd" || c_child.first == "d"))
              continue;
            // OSPRay SciVis renderer uses kd, d, ks and ns params
            if (h.first == "scivis" &&
                !(c_child.first == "kd" || c_child.first == "d" ||
                  c_child.first == "ks" || c_child.first == "ns"))
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

  }  // namespace sg
} // namespace ospray
