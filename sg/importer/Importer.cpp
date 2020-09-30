// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Importer.h"

#include "../JSONDefs.h"

namespace ospray {
namespace sg {

OSPSG_INTERFACE std::map<std::string, std::string> importerMap = {
    {"obj", "importer_obj"},
    {"gltf", "importer_gltf"},
    {"glb", "importer_gltf"},
    {"raw", "importer_raw"},
    {"vdb", "importer_vdb"}};

Importer::Importer() {}

NodeType Importer::type() const
{
  return NodeType::IMPORTER;
}

void Importer::importScene() {}

OSPSG_INTERFACE void importScene(
    std::shared_ptr<StudioContext> context, rkcommon::FileName &fileName)
{
  std::cout << "this is importScene!" << std::endl;
  context->filesToImport.clear();
  std::ifstream sgFile(fileName.str());
  if (!sgFile) {
    std::cerr << "Could not open " << fileName << " for reading" << std::endl;
    return;
  }

  nlohmann::json j;
  sgFile >> j;
  sg::NodePtr lights;

  auto &jWorld = j["world"];
  for (auto &jChild : jWorld["children"]) {
    switch (jChild["type"].get<NodeType>()) {
    case NodeType::IMPORTER:
      context->filesToImport.push_back(jChild["filename"]);
      break;
    case NodeType::LIGHTS:
      lights = createNode(jChild);
      break;
    default:
      break;
    }
  }

  context->refreshScene(true);

  context->frame->child("world").add(lights);

  CameraState cs = j["camera"];
  context->setCameraState(cs);
  context->updateCamera();

  for (auto &jChild : jWorld["children"]) {
    if (jChild["type"] == NodeType::IMPORTER) {
      auto &imp = context->frame->child("world").child(jChild["name"]);
      auto &jXfm = jChild["children"][0];
      // set the correct value for the transform
      imp.child(jXfm["name"]) = jXfm["value"].get<AffineSpace3f>();
    }
  }
}

} // namespace sg
} // namespace ospray
