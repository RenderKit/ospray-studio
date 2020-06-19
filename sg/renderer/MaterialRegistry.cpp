// ======================================================================== //
// Copyright 2020 Intel Corporation                                    //
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

#include "MaterialRegistry.h"

namespace ospray::sg {
  MaterialRegistry::MaterialRegistry()
  {}

  void MaterialRegistry::addNewSGMaterial(std::string matType)
  {
    if (!hasChild(matType))
      createChild(matType, matType);
  }

  void MaterialRegistry::refreshMaterialList(const std::string &matType, const std::string &rType)
  {
    cppMaterialList.clear();

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

}  // namespace ospray::sg
