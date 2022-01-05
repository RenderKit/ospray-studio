// Copyright 2009-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

// sg
#include "sg/Node.h"
#include "sg/renderer/MaterialRegistry.h"
#include "sg/scene/Animation.h"
#include "sg/texture/Texture2D.h"
#include "sg/scene/volume/Volume.h"
#include "sg/generator/Generator.h"
// rkcommon
#include "rkcommon/os/FileName.h"
#include "rkcommon/utility/StringManip.h"

#include "app/ospStudio.h"

namespace ospray {
namespace sg {

// map of asset Titles and corresponding original importer nodes
typedef std::map<std::string, NodePtr> AssetsCatalogue;

enum InstanceConfiguration {
    STATIC = 0,
    DYNAMIC= 1,
    COMPACT = 2,
    ROBUST = 3     
};

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

  inline std::vector<NodePtr> *getCameraList()
  {
    return cameras;
  }

  inline void setAnimationList(std::vector<sg::Animation> &_animations)
  {
    animations = &_animations;
  }

  inline std::vector<sg::Animation> *getAnimationList()
  {
    return animations;
  }

  inline void setLightsManager(NodePtr _lightsManager)
  {
    lightsManager = _lightsManager;
  }

  inline NodePtr getLightsManager()
  {
    return lightsManager;
  }

  inline void setFb(sg::FrameBuffer &_fb)
  {
    fb = &_fb;
  }

  inline sg::FrameBuffer *getFb()
  {
    return fb;
  }

  inline void setVolumeParams(NodePtr vp) {
    volumeParams = vp;
  }

  inline NodePtr getVolumeParams() {
    return volumeParams;
  }

  inline void setInstanceConfiguration(InstanceConfiguration _ic) {
    ic = _ic;
  }

  inline InstanceConfiguration getInstanceConfiguration() {
    return ic;
  }

  inline void setArguments(int _argc, char** _argv){
    argc = _argc;
    argv = _argv;
  }

  float pointSize{0.0f};

 protected:
  rkcommon::FileName fileName;
  std::shared_ptr<sg::MaterialRegistry> materialRegistry = nullptr;
  std::vector<NodePtr> *cameras = nullptr;
  std::vector<sg::Animation> *animations = nullptr;
  bool importCameras{false};
  NodePtr volumeParams;
  NodePtr lightsManager;
  sg::FrameBuffer *fb{nullptr};
  InstanceConfiguration ic{STATIC};
  int argc{0};
  char ** argv{nullptr};
};

// global assets catalogue
extern OSPSG_INTERFACE AssetsCatalogue cat;
extern OSPSG_INTERFACE std::map<std::string, std::string> importerMap;

// Providing a unique transform instance as root to add existing imported model to, 
// should probably be the responsibility of the calling routine
inline std::shared_ptr<Importer> getImporter(
    NodePtr root, rkcommon::FileName fileName)
{
  std::string baseName = fileName.base();
  auto fnd = importerMap.find(rkcommon::utility::lowerCase(fileName.ext()));
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
    } while (root->hasChild(nodeName));

    auto instanceXfm = createNode(nodeName, "transform");

    // Add all children of the original rootXfm to this instanceXfm
    for (auto &g : rootXfmNode.children())
      instanceXfm->add(g.second);

    root->add(instanceXfm);

    return nullptr;

  } else {
    auto importNode = createNodeAs<Importer>(nodeName, importer);
    if (importer == "importer_raw") {
      std::cout << "Loading volumes with default volume parameters ..."
                << std::endl;
      auto volumeFile = fnd->first;
      bool structured = (volumeFile == "structured") || (volumeFile == "raw");
      auto vp = std::make_shared<VolumeParams>(structured);
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
