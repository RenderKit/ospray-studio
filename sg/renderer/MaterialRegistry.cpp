// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "MaterialRegistry.h"

namespace ospray {
  namespace sg {

  MaterialRegistry::MaterialRegistry()
  {
    // ensure there's one OBJ material in the Registry as the default material
    addNewSGMaterial("obj");
  }

  void MaterialRegistry::addNewSGMaterial(std::string matType)
  {
    if (!hasChild(matType))
      createChild(matType, matType);
  }

  /*
  void MaterialRegistry::refreshMaterialList(const std::string &matType, const std::string &rType)
  {
    cppMaterialList.clear();

    //puts the provided material first in the list so that it will be used by default
    //however doing so will also break other items because their indexes will be wrong
    for (auto mat_it = sgMaterialList.begin(); mat_it != sgMaterialList.end(); ++mat_it) {
      auto &materialNode = *(*mat_it);
      if (materialNode.name() == matType) {
        auto mat_x = *mat_it;
        sgMaterialList.erase(mat_it);
        sgMaterialList.insert(sgMaterialList.begin(), mat_x);
        break;
      }
    }

    for (auto mat_it = sgMaterialList.begin(); mat_it != sgMaterialList.end(); ++mat_it) {
      auto &materialNode = *(*mat_it);
      auto &ospHandleNode = materialNode.child("handles").child(rType);
      auto &cppMaterial   = ospHandleNode.valueAs<cpp::Material>();
      cppMaterialList.push_back(cppMaterial);
    }
  }
  */

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
