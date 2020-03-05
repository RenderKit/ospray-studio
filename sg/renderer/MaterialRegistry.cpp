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
  {
    auto mr   = createNode("materialRegistry");
    setValue(mr);
  }
 
  MaterialRegistry::MaterialRegistry(const std::vector<std::string> g_matTypes)
  {
    auto mr   = createNode("materialRegistry");
    for (auto mat_it = g_matTypes.begin(); mat_it != g_matTypes.end(); ++mat_it) {
      mr->createChild(*mat_it, *mat_it);
    }
    setValue(mr);
  }

  void MaterialRegistry::addNewOSPMaterial(std::string matType)
  {
    auto &mr = valueAs<NodePtr>();

    if (!mr->hasChild(matType))
      mr->createChild(matType, matType);
  }

/////////// TODO:setup better importer and material registry interface

  void MaterialRegistry::refreshMaterialList(const std::string &matType, const std::string &rType)
  {
    auto &mr = valueAs<NodePtr>();
    materialList.clear();

    for (auto mat_it = materialMap.begin(); mat_it != materialMap.end(); ++mat_it) {
      auto &materialNode = *(*mat_it);
      if (materialNode.name() == matType) {
        auto mat_x = *mat_it;
        materialMap.erase(mat_it);
        materialMap.insert(materialMap.begin(), mat_x);
        break;     
      }
    }

    for (auto mat_it = materialMap.begin(); mat_it != materialMap.end(); ++mat_it) {
      auto &materialNode = *(*mat_it);      
      auto &ospHandleNode = materialNode.child("handles").child(rType);
      auto &cppMaterial   = ospHandleNode.valueAs<cpp::Material>();
      materialList.push_back(cppMaterial);
    }
  }

  void MaterialRegistry::updateMaterialRegistry(const std::string &rType) {
    auto &mr = valueAs<NodePtr>();
    mr->traverse<sg::GenerateOSPRayMaterials>(rType);

    mr->commit();
  }

  void MaterialRegistry::removeImportedMats(const std::string &rType)
  {
    auto &mr = valueAs<NodePtr>();

    if (importedMatNames.size() != 0) {
        for (auto & m : importedMatNames) {
          mr->remove(m);
      }
    }

    importedMatNames.clear();
    updateMaterialList(rType);
  }

  void MaterialRegistry::updateMaterialList(const std::string &rType) {
    updateMaterialRegistry(rType);
    auto &mr = valueAs<NodePtr>();
    materialMap.clear();
    materialList.clear();
    auto &mats = mr->children();

    for (auto &m : mats) {
      auto &matHandle = m.second->child("handles");

      if (!matHandle.hasChild(rType))
        return;
      auto sgMaterial      = m.second->nodeAs<sg::Material>();
      auto &ospHandleNode = matHandle.child(rType);
      auto &cppMaterial   = ospHandleNode.valueAs<cpp::Material>();
      materialMap.push_back(sgMaterial);
      materialList.push_back(cppMaterial);
      }

      if (importedMatNames.size() != 0) {
        for (auto newMat : importedMatNames) {
          auto &matChild  = mr->child(newMat);
          auto &matHandle = matChild.child("handles");
          if (!matHandle.hasChild(rType))
            return;
          auto sgMaterial      = matChild.nodeAs<sg::Material>();
          auto &ospHandleNode = matHandle.child(rType);
          auto &cppMaterial   = ospHandleNode.valueAs<cpp::Material>();
          materialMap.insert(materialMap.begin(), sgMaterial);
          materialList.insert(materialList.begin(), cppMaterial);
        }
      }
    } 

}  // namespace ospray::sg
