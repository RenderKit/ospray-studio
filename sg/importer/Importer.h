// Copyright 2009-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../Node.h"
#include "rkcommon/os/FileName.h"
#include "sg/renderer/MaterialRegistry.h"
#include "sg/scene/Animation.h"
#include "sg/texture/Texture2D.h"

#include "../../app/ospStudio.h"

namespace ospray {
namespace sg {

// map of asset Titles and corresponding original importer nodes
typedef std::map<std::string, NodePtr> AssetsCatalogue;

// global assets catalogue
extern AssetsCatalogue cat;

typedef struct {
  vec3i dimensions;
  vec3f gridSpacing{0.02f};
  vec3f gridOrigin{-1.f};
  int voxelType;
} VolumeParams;

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

  inline void setCameraList(std::vector<NodePtr> &_cameras)
  {
    cameras = &_cameras;
    importCameras = true;
  }

  inline void setAnimationList(std::vector<sg::Animation> &_animations)
  {
    animations = &_animations;
  }

  inline void setVolumeParams(VolumeParams *_p)
  {
    p = _p;
    hasVolumeParams = true;
  }

  inline void setLightsManager(NodePtr _lightsManager)
  {
    lightsManager = _lightsManager;
  }

  inline VolumeParams* setDefaultParams(bool structured) {
    if (structured) {
      defaultParams.voxelType = int(OSP_FLOAT);
      defaultParams.dimensions = vec3i(18, 25, 18);
      defaultParams.gridOrigin = vec3f(-1.f);
      defaultParams.gridSpacing = vec3f(2.f / 100);
    } else {
      defaultParams.voxelType = int(OSP_FLOAT);
      defaultParams.dimensions = vec3i(180, 180, 180);
      defaultParams.gridOrigin = vec3f(0);
      defaultParams.gridSpacing = vec3f(1, 1, 1);
    }
    return &defaultParams;
  }

  VolumeParams defaultParams;
  bool hasVolumeParams{false};

 protected:
  rkcommon::FileName fileName;
  std::shared_ptr<sg::MaterialRegistry> materialRegistry = nullptr;
  std::vector<NodePtr> *cameras = nullptr;
  std::vector<sg::Animation> *animations = nullptr;
  bool importCameras{false};
  VolumeParams *p{nullptr};
  NodePtr lightsManager;
};

extern OSPSG_INTERFACE std::map<std::string, std::string> importerMap;

// Providing a unique transform instance as root to add existing imported model to, 
// should probably be the responsibility of the calling routine
inline std::shared_ptr<Importer> getImporter(
    NodePtr root, rkcommon::FileName fileName)
{
  std::string baseName = fileName.name();
  auto fnd = importerMap.find(fileName.ext());
  if (fnd == importerMap.end()) {
    std::cout << "No importer for " << fileName << std::endl;
    return nullptr;
  }
  std::string importer = fnd->second;
  std::string nodeName = baseName + "_importer";

  if (cat.find(baseName) != cat.end()) {
    // Existing import, instance it!
    std::cout << "Instancing: " << nodeName << std::endl;

    // Importer node and its rootXfm
    auto &origNode = cat[baseName];
    std::string rootXfmName = baseName + "_rootXfm";

    if (!origNode->hasChild(rootXfmName)) {
      std::cout << "!!! error... importer rootXfm is missing?!" << std::endl;
    }
    auto &rootXfmNode = origNode->child(rootXfmName);

    // Create a unique instanceXfm nodeName
    auto count = 1;
    do {
      nodeName = baseName + "_instanceXfm_" + std::to_string(count++);
    } while (origNode->hasChild(nodeName));

    auto instanceXfm = createNode(nodeName, "transform");

    // Add all children of the original rootXfm to this instanceXfm
    for (auto &g : rootXfmNode.children())
      instanceXfm->add(g.second);

    root->add(instanceXfm);

    return nullptr;

  } else {
    auto importNode = createNodeAs<Importer>(nodeName, importer);
    if(importer == "importer_raw" && !importNode->hasVolumeParams) {
      std::cout
        << "Loading volumes with default volume parameters ..."
        << std::endl;
      auto volumeFile = fnd->first;
      bool structured = (volumeFile == "structured") || (volumeFile == "raw");
      auto vp = importNode->setDefaultParams(structured);
      importNode->setVolumeParams(vp);
    }
    importNode->setFileName(fileName);
    cat.insert(AssetsCatalogue::value_type(baseName, importNode));
    root->add(importNode);
    return importNode;
  }
}

inline void clearAssets()
{
  cat.clear();
}

// for loading scene (.sg) files
OSPSG_INTERFACE void importScene(
    std::shared_ptr<StudioContext> context, rkcommon::FileName &fileName);

} // namespace sg
} // namespace ospray
