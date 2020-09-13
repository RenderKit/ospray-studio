// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../Node.h"
#include "rkcommon/os/FileName.h"
#include "sg/renderer/MaterialRegistry.h"
#include  "sg/texture/Texture2D.h"

namespace ospray {
  namespace sg {

  struct OSPSG_INTERFACE Importer : public Node
  {
    Importer();
    virtual ~Importer() = default;

    NodeType type() const override;

    virtual void importScene(
        std::shared_ptr<sg::MaterialRegistry> materialRegistry);

    virtual void importScene();
  };

  static const std::map<std::string, std::string> importerMap = {
      {"obj", "importer_obj"},
      {"gltf", "importer_gltf"},
      {"glb", "importer_gltf"},
      {"raw", "importer_raw"}};

  inline std::string getImporter(rkcommon::FileName fileName)
  {
    auto fnd = importerMap.find(fileName.ext());
    if (fnd == importerMap.end()) {
      std::cout << "No importer for " << fileName << std::endl;
      return "";
    }

    std::string importer = fnd->second;
    std::string nodeName = "importer" + fileName.base();

    // auto &node = createNodeAs<sg::Importer>(nodeName, importer);
    // child("file") = fileName.base();
    return importer;
  }

  }  // namespace sg
} // namespace ospray
