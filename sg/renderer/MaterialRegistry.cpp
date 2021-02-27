// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "MaterialRegistry.h"

namespace ospray {
namespace sg {

MaterialRegistry::MaterialRegistry()
{
  // Renderer type the current materials were created for
  createChild("rType", "string", "renderer type", std::string(""));
  child("rType").setSGOnly();

  //---------------------------------------------------------------------
  // All non-material nodes *must* be created above this line
  nonMaterialCount = children().size();

  // ensure there's one OBJ material in the Registry as the default material
  // use obj since it works with SciVis, AO, and Pathtrace renderers
  auto node = createNode("sgDefault", "obj");
  auto &mat = *node;
  // Give it some default parameters
  mat.createChild("kd", "rgb", "diffuse color", vec3f(0.8f));
  mat.createChild("ks", "rgb", "specular color", vec3f(0.f));
  mat.createChild("ns", "float", "shininess [2-10e4]", 10.f);
  mat.createChild("d", "float", "opacity [0-1]", 1.f);
  mat.createChild("tf", "rgb", "transparency filter color", vec3f(0.f));
  add(node);

  mat.child("ns").setMinMax(2.f, 10000.f);
  mat.child("d").setMinMax(0.f, 1.f);
}

void MaterialRegistry::updateRendererType()
{
  // Materials are renderer_type dependent, must be rcreated for current type.
  auto &frame = parents().front();
  auto &renderer = frame->childAs<sg::Renderer>("renderer");
  auto rType = renderer["type"].valueAs<std::string>();

  // This will modify the material registry causing it to recreate the list
  // for the current renderer type, at commit.
  child("rType").setValue(rType);
}

void MaterialRegistry::preCommit()
{
  auto rType = child("rType").valueAs<std::string>();

  traverse<sg::GenerateOSPRayMaterials>(rType);
  cppMaterialList.clear();

  // debug renderers don't handle any materials
  if (rType == "debug")
    return;

  auto defaultMaterial =
      child("sgDefault")["handles"].child(rType).nodeAs<sg::Material>();
  auto &defaultCppMaterial = defaultMaterial->valueAs<cpp::Material>();

  for (auto &m : children()) {
    auto &mNode = *(m.second);
    if (mNode.sgOnly())
      continue;

    // Make sure each material handles the current renderer type.  If it
    // doesn't, add the default material to keep all the indices in order.
    // XXX soon, we'll generalize materials so that every material will make a
    // 'best attempt' to handle all renderers.
    if (mNode.hasChild("handles") && mNode["handles"].hasChild(rType)) {
      auto material = mNode["handles"].child(rType).nodeAs<sg::Material>();
      auto &cppMaterial = material->valueAs<cpp::Material>();
      cppMaterialList.push_back(cppMaterial);
    } else
      cppMaterialList.push_back(defaultCppMaterial);
  }
}

void MaterialRegistry::postCommit()
{
  auto &frame = parents().front();
  auto &renderer = frame->childAs<sg::Renderer>("renderer");

  if (!cppMaterialList.empty())
    renderer.handle().setParam("material", cpp::CopiedData(cppMaterialList));
  else
    renderer.handle().removeParam("material");

  renderer.handle().commit();
}

OSP_REGISTER_SG_NODE_NAME(MaterialRegistry, materialRegistry);

} // namespace sg
} // namespace ospray
