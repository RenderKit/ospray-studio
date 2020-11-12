// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../Node.h"
#include "rkcommon/os/FileName.h"
#include "sg/renderer/MaterialRegistry.h"
#include "sg/texture/Texture2D.h"

#include "../../app/ospStudio.h"

namespace ospray {
namespace sg {

// map of asset Titles and corresponding original importer nodes
typedef std::map<std::string, NodePtr> AssetsCatalogue;

struct OSPSG_INTERFACE Importer : public Node
{
  Importer();
  virtual ~Importer() = default;

  NodeType type() const override;

  virtual void importScene();

  inline const rkcommon::FileName getFileName() const
  {
    return fileName;
  }

  inline void setFileName(rkcommon::FileName _fileName)
  {
    fileName = _fileName;
  }

  inline void setMaterialRegistry(
      std::shared_ptr<sg::MaterialRegistry> _registry)
  {
    materialRegistry = _registry;
  }

  inline void setTimesteps(std::vector<float> &_timesteps)
  {
    timesteps = &_timesteps;
    animate = true;
  }

  inline void setCameraList(std::vector<NodePtr> &_cameras)
  {
    cameras = &_cameras;
    importCameras = true;
  }

 protected:
  rkcommon::FileName fileName;
  std::shared_ptr<sg::MaterialRegistry> materialRegistry = nullptr;
  std::vector<float> *timesteps = nullptr;
  std::vector<NodePtr> *cameras = nullptr;
  bool importCameras{false};
  bool animate{false};
};

extern OSPSG_INTERFACE std::map<std::string, std::string> importerMap;

inline Importer *getImporter(NodePtr root, rkcommon::FileName fileName)
{
  std::string baseName = fileName.name();
  auto fnd = importerMap.find(fileName.ext());
  if (fnd == importerMap.end()) {
    std::cout << "No importer for " << fileName << std::endl;
    return nullptr;
  }

  std::string importer = fnd->second;
  std::string nodeName = baseName + "_importer";

  Importer *importNode = nullptr;

  if (root->hasChild(nodeName)) {
    // Existing import, instance it!
    std::cout << "Instancing: " << nodeName << std::endl;

    // Importer node and its rootXfm
    auto &origNode = root->child(nodeName);
    std::string rootXfmName = baseName + "_rootXfm";
    if (!origNode.hasChild(rootXfmName)) {
      std::cout << "!!! error... importer rootXfm is missing?!" << std::endl;
      return nullptr;
    }
    auto &rootXfmNode = origNode.child(rootXfmName);

    // Create a unique instanceXfm nodeName
    auto count = 1;
    do {
      nodeName = baseName + "_instanceXfm_" + std::to_string(count++);
    } while (origNode.hasChild(nodeName));

    auto instanceXfm = createNode(nodeName, "Transform", affine3f{one});

    // Add all children of the original rootXfm to this instanceXfm
    for (auto &g : rootXfmNode.children())
      instanceXfm->add(g.second);

    // add instance to importer parent
    origNode.add(instanceXfm);

    // Everything is done, no need to return a node.

  } else {
    // Import
    importNode = &root->createChildAs<sg::Importer>(nodeName, importer);
    importNode->setFileName(fileName);
  }

  return importNode;
}

// for loading scene (.sg) files
OSPSG_INTERFACE void importScene(
    std::shared_ptr<StudioContext> context, rkcommon::FileName &fileName);

} // namespace sg
} // namespace ospray
