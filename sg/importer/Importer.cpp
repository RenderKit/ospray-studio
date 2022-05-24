// Copyright 2009-2022 Intel Corporation
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
    {"vdb", "importer_vdb"},
    {"pcd", "importer_pcd"},
    {"pvol", "importer_pvol"}};

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

  std::map<std::string, JSON> jImporters;
  std::map<std::string, JSON> jGenerators;
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

              jImporters[jChild["name"]] = jChild;
              break;
            }
          } else
            std::cerr << "Unable to find " << fileName << std::endl;
        }
      } break;
      case NodeType::GENERATOR:
        // Handle generators in the world
        jGenerators[jChild["name"]] = jChild;
        break;
      case NodeType::LIGHTS:
        // Handle lights in either the (world) or the lightsManager
        lights = createNodeFromJSON(jChild);
        break;
      default:
        break;
      }
    }
  }

  //
  // Generator Objects
  //
  for (auto &jGenerator : jGenerators) {
    auto &jG = jGenerator.second;
    std::string name = jG["name"];
    std::string subType = jG["subType"];
    std::cout << "  Generator: " << name << ":" << subType << std::endl;

    auto world = context->frame->childNodeAs<sg::Node>("world");
    auto gen = &world->createChildAs<sg::Generator>(name, subType);
    if (gen) {
      // Allow the generator access to the material registry
      gen->setMaterialRegistry(context->baseMaterialRegistry);

      // Parse entire generator json for children.  Only add valid values
      auto children = createNodeFromJSON(jG)->children();
      std::function<void(Node &, const FlatMap<std::string, NodePtr> &)>
          setJsonValues = [&setJsonValues](Node &node,
                          const FlatMap<std::string, NodePtr> &children) {
            for (auto &child : children) {
              auto &cn = child.second;
              if (cn->value().valid()) {
                if (node.hasChild(cn->name()))
                  node.child(cn->name()) = cn->value();
                else
                  node.add(cn);
              }
              // recurse if there are children
              if (!cn->children().empty())
                setJsonValues(node.child(cn->name()), cn->children());
            }
          };

      setJsonValues(*gen, children);

      // Commit any parameter changes
      gen->commit();
    }
  }

  // refreshScene imports all filesToImport
  if (!jGenerators.empty() || !context->filesToImport.empty())
    context->refreshScene(true);

  // Any lights in the scenefile World are added here
  if (lights) {
    for (auto &light : lights->children())
      context->lightsManager->addLight(light.second);
  }

  // If the sceneFile contains a lightsManager, add those lights here
  if (j.contains("lightsManager")) {
    auto &jLights = j["lightsManager"];
    for (auto &jLight : jLights["children"])
      context->lightsManager->addLight(createNodeFromJSON(jLight));
  }

  // If the sceneFile contains materials, parse them here, after the model has
  // loaded. These parameters will overwrite materials in the model file.
  if (j.contains("materialRegistry")) {
    sg::NodePtr materials = createNodeFromJSON(j["materialRegistry"]);

    for (auto &mat : materials->children()) {
      // skip non-material nodes (e.g. renderer type)
      if (mat.second->type() != NodeType::MATERIAL)
        continue;
      // kill old parent (from previous session); avoids a segfault when
      // modifying parameters from loaded materials
      mat.second->killAllParents();

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
    context->finalCameraView = std::make_shared<affine3f>(cs.cameraToWorld);
    context->updateCamera();
  }

  //
  // After import, correctly apply transform on import nodes
  // (must happen after refreshScene)
  //
  auto world = context->frame->childNodeAs<sg::Node>("world");

  //
  // File Importer Objects
  // (already imported, just need to apply scene file transform)
  //
  for (auto &jImport : jImporters) {
    // lambda, find node by name
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

    auto importNode = findFirstChild(world, jImport.first);
    auto jNode = jImport.second["children"][0];
    if (importNode && jNode["subType"] == "transform") {
      // should be associated xfm node
      auto childName = jNode["name"];
      Node &xfmNode = importNode->child(childName);

      auto xfm = createNodeFromJSON(jNode);
      if (xfm) {
        xfmNode = xfm->value(); // assigns base affine3f value
        // Update the xfm rotation, translation and scale values
        xfmNode["rotation"] = xfm->child("rotation").valueAs<quaternionf>();
        xfmNode["translation"] = xfm->child("translation").valueAs<vec3f>();
        xfmNode["scale"] = xfm->child("scale").valueAs<vec3f>();
      }
    }
  }
}

// global assets catalogue
AssetsCatalogue cat;

} // namespace sg
} // namespace ospray
