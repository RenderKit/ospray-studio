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
    {"structured", "importer_raw"},
    {"spherical", "importer_raw"},
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
  std::cout << "Importing a scene" << std::endl;
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
      lights = createNodeFromJSON(jChild);
      break;
    default:
      break;
    }
  }

  context->refreshScene(true);

  if (lights == nullptr) {
    std::cerr << "Scene file '" << fileName
              << "' has no lights! Is this file correct?" << std::endl;
  } else {
    context->frame->child("world").add(lights);
  }

  CameraState cs = j["camera"];
  context->setCameraState(cs);
  context->updateCamera();

  std::function<sg::NodePtr(const sg::NodePtr, const std::string &)>
      findFirstChild = [&findFirstChild](const sg::NodePtr root,
                           const std::string &name) -> sg::NodePtr {
    sg::NodePtr found = nullptr;

    // Quick shallow top-level search first
    for (auto child : root->children())
      if (child.first == name)
        return child.second;

    // Next level, deeper search if not found
    for (auto child : root->children()) {
      found = findFirstChild(child.second, name);
      if (found)
        return found;
    }

    return found;
  };

  for (auto &jChild : jWorld["children"]) {
    if (jChild["type"] == NodeType::IMPORTER) {
      auto imp = findFirstChild(
          context->frame->childNodeAs<sg::Node>("world"), jChild["name"]);
      auto &jXfm = jChild["children"][0];
      // set the correct value for the transform
      if (imp)
        imp->child(jXfm["name"]) = jXfm["value"].get<AffineSpace3f>();
    }
  }
}

} // namespace sg
} // namespace ospray
