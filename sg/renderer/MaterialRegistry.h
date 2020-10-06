// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once
#include <vector>
#include "../Node.h"
#include "../visitors/GenerateOSPRayMaterials.h"

namespace ospray {
  namespace sg {

  struct OSPSG_INTERFACE MaterialRegistry : public Node
  {
    MaterialRegistry();

    ~MaterialRegistry() override = default;

    void addNewSGMaterial(std::string matType);

    void createCPPMaterials(const std::string &rType);

    void updateMaterialList(const std::string &rType);

    void refreshMaterialList(const std::string &matType, const std::string &rType);

    void rmMatImports();

    std::vector<std::string> matImportsList;

    std::vector<cpp::Material> cppMaterialList;

    std::vector<std::shared_ptr<sg::Material>> sgMaterialList;

  };

  }  // namespace sg
} // namespace ospray
