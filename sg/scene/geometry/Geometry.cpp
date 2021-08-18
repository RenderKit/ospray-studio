// Copyright 2009-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Geometry.h"

namespace ospray {
namespace sg {

Geometry::Geometry(const std::string &osp_type)
{
  setValue(cpp::Geometry(osp_type));
  createChild("isClipping", "bool", false);
  createChild("visible", "bool", true);
  createChild("invertNormals", "bool", false);

  child("isClipping").setSGOnly();
  child("visible").setSGOnly();
  child("invertNormals").setSGOnly();
}

NodeType Geometry::type() const
{
  return NodeType::GEOMETRY;
}

void Geometry::postCommit()
{
  // call baseClass postCommit to finish geometry node first
  OSPNode::postCommit();

  // Create GeometricModel
  model = std::make_shared<cpp::GeometricModel>(
      cpp::GeometricModel(valueAs<cpp::Geometry>()));

  if (hasChild("material")) {
    if (child("material").valueIsType<cpp::SharedData>())
      model->setParam("material", child("material").valueAs<cpp::SharedData>());
    else if (child("material").valueIsType<cpp::CopiedData>())
      model->setParam("material", child("material").valueAs<cpp::CopiedData>());
    else
      model->setParam("material",
          cpp::CopiedData(std::vector<uint32_t>{
              child("material").valueAs<unsigned int>()}));
  }

  if (hasChild("color")) {
    if (child("color").valueIsType<cpp::SharedData>())
      model->setParam("color", child("color").valueAs<cpp::SharedData>());
    else if (child("color").valueIsType<cpp::CopiedData>())
      model->setParam("color", child("color").valueAs<cpp::CopiedData>());
    else
      model->setParam("color",
          cpp::CopiedData(std::vector<vec4f>{child("color").valueAs<vec4f>()}));
  }

  if (hasChild("invertNormals")) {
    bool val = child("invertNormals").valueAs<bool>();
    model->setParam("invertNormals", val);
  }

  model->commit();

  std::string type =
      child("isClipping").valueAs<bool>() ? "clippingGeometry" : "geometry";

  group = std::make_shared<cpp::Group>(cpp::Group());
  group->setParam(type, cpp::CopiedData(*model));
  group->commit();
}

} // namespace sg
} // namespace ospray
