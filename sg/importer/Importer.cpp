// Copyright 2009-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Importer.h"
#include "sg/visitors/PrintNodes.h"

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

void Importer::importScene() {
}

OSPSG_INTERFACE void importScene(
    std::shared_ptr<StudioContext> context, rkcommon::FileName &sceneFileName)
{
  std::cout << "Importing a scene" << std::endl;
  context->filesToImport.clear();
  std::ifstream sgFile(sceneFileName.str());
  if (!sgFile) {
    std::cerr << "Could not open " << sceneFileName << " for reading"
              << std::endl;
    return;
  }

  JSON j;
  sgFile >> j;

  std::map<std::string, AffineSpace3f> importer_xfms = {};
  sg::NodePtr lights;

  // If the sceneFile contains a world (importers and lights), parse it here
  // (must happen before refreshScene)
  if (j.contains("world")) {
    auto &jWorld = j["world"];
    for (auto &jChild : jWorld["children"]) {

      // Import either the old-type enum directly, or the new-type enum STRING
      NodeType nodeType = jChild["type"].is_string()
          ? NodeTypeFromString[jChild["type"]]
          : jChild["type"].get<NodeType>();

      switch (nodeType) {
      case NodeType::IMPORTER: {
        FileName fileName = std::string(jChild["filename"]);

        // Try a couple different paths to find the file before giving up
        std::vector<std::string> possibleFileNames = {fileName, // as imported
            sceneFileName.path() + fileName.base(), // in scenefile directory
            fileName.base(), // in local directory
            ""};

        for (auto tryFile : possibleFileNames) {
          if (tryFile != "") {
            std::ifstream f(tryFile);
            if (f.good()) {
              context->filesToImport.push_back(tryFile);

              auto &jXfm = jChild["children"][0];
              importer_xfms[jXfm["name"]] = jXfm["value"].get<AffineSpace3f>();
              break;
            }
          } else
            std::cerr << "Unable to find " << fileName << std::endl;
        }
      } break;
      case NodeType::LIGHTS:
        // Handle lights in either the (world) or the lightsManager
        lights = createNodeFromJSON(jChild);
        break;
      default:
        break;
      }
    }
  }

  // refreshScene imports all filesToImport
  if (!context->filesToImport.empty())
    context->refreshScene(true);

  // If the sceneFile contains lightsManager light definitions, parse it here
  if (j.contains("lightsManager"))
    lights = createNodeFromJSON(j["lightsManager"]);

  // Add lights to lightsManager (either from world or lightsManager defn's)
  if (lights) {
    // If the scene contains lights, remove the default ambient.
    context->lightsManager->removeLight("ambient");
    for (auto &light : lights->children())
      context->lightsManager->addLight(light.second);
  }

  // If the sceneFile contains materials, parse them here, after the model has
  // loaded. These parameters will overwrite materials in the model file.
  if (j.contains("materialRegistry")) {
    sg::NodePtr materials = createNodeFromJSON(j["materialRegistry"]);

    for (auto &mat : materials->children()) {

      // XXX temporary workaround.  Just set params on existing materials.
      // Prevents loss of texture data.  Will be fixed when textures can reload.

      // Modify existing material or create new material
      // (account for change of material type)
      if (context->baseMaterialRegistry->hasChild(mat.first)
          && context->baseMaterialRegistry->child(mat.first).subType()
              == mat.second->subType()) {
        auto &bMat = context->baseMaterialRegistry->child(mat.first);

        for (auto &param : mat.second->children()) {
          auto &p = *param.second;

          // This is a generated node value and can't be imported
          if (param.first == "handles")
            continue;

          // Modify existing param or create new params
          if (bMat.hasChild(param.first))
            bMat[param.first] = p.value();
          else
            bMat.createChild(
                param.first, p.subType(), p.description(), p.value());
        }
      } else
        context->baseMaterialRegistry->add(mat.second);
    }
    // refreshScene imports all filesToImport and updates materials
    context->refreshScene(true);
  }

  // If the sceneFile contains a camera location
  // (must happen after refreshScene)
  if (j.contains("camera")) {
    CameraState cs = j["camera"];
    context->setCameraState(cs);
    context->updateCamera();
  }

  // after import, correctly apply transform import nodes
  // (must happen after refreshScene)
  auto world = context->frame->childNodeAs<sg::Node>("world");
  for (auto &xfm : importer_xfms) {
    // lamdba, find node by name
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

    auto importNode = findFirstChild(world, xfm.first);
    if (importNode)
      importNode->setValue(xfm.second);
  }
}

} // namespace sg
} // namespace ospray
