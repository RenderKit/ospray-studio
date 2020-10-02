// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "MaterialRegistry.h"

namespace ospray {
  namespace sg {

  MaterialRegistry::MaterialRegistry()
  {
    // ensure there's one OBJ material in the Registry as the default material
    // use obj since it works with both SciVis and Pathtrace renderers
    addNewSGMaterial("obj");
  }

  void MaterialRegistry::addNewSGMaterial(std::string matType)
  {
    if (!hasChild(matType)) {
      auto node = createNode("sgDefault", matType);
      auto &mat = *node;
      // Give it some editable parameters
      mat.createChild("kd", "rgb", "diffuse color", vec3f(0.8f));
      mat.createChild("ks", "rgb", "specular color", vec3f(0.f));
      mat.createChild("ns", "float", "shininess [2-10e4]", 10.f);
      mat.createChild("d", "float", "opacity [0-1]", 1.f);
      mat.createChild("tf", "rgb", "transparency filter color", vec3f(0.f));
      add(node);

      mat.child("ns").setMinMax(2.f,10000.f);
      mat.child("d").setMinMax(0.f,1.f);
    }
  }

  void MaterialRegistry::createCPPMaterials(const std::string &rType) 
  {
    this->traverse<sg::GenerateOSPRayMaterials>(rType);
    this->commit();
  }

  void MaterialRegistry::rmMatImports()
  {
    if (matImportsList.size() != 0) {
      for (auto &m : matImportsList) {
        this->remove(m);
      }
    }
    matImportsList.clear();
  }

  void MaterialRegistry::updateMaterialList(const std::string &rType) 
  {
    createCPPMaterials(rType);
    sgMaterialList.clear();
    cppMaterialList.clear();
    auto &mats = this->children();

    for (auto &m : mats) {
      auto &matHandle = m.second->child("handles");
      auto sgMaterial = m.second->nodeAs<sg::Material>();
      sgMaterialList.push_back(sgMaterial);
      if (m.second->nodeAs<Material>()->osprayMaterialType() == "obj" ||
          rType == "pathtracer") {
        auto &cppMaterial = matHandle.child(rType).valueAs<cpp::Material>();
        cppMaterialList.push_back(cppMaterial);
      }
    }
  } 

  OSP_REGISTER_SG_NODE_NAME(MaterialRegistry, materialRegistry);

  }  // namespace sg
} // namespace ospray
