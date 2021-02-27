// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once
#include <vector>
#include "sg/Node.h"
#include "sg/renderer/Renderer.h"
#include "sg/visitors/GenerateOSPRayMaterials.h"

namespace ospray {
namespace sg {

struct OSPSG_INTERFACE MaterialRegistry : public Node
{
  MaterialRegistry();
  ~MaterialRegistry() override = default;

  virtual void preCommit() override;
  virtual void postCommit() override;

  void updateRendererType();

  inline uint32_t baseMaterialOffSet()
  {
    return children().size() - nonMaterialCount;
  }

 private:
  std::vector<cpp::Material> cppMaterialList;
  std::string rType{""};

  uint32_t nonMaterialCount{0};
};

} // namespace sg
} // namespace ospray
