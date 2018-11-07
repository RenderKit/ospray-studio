// ======================================================================== //
// Copyright 2009-2018 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#include "ospray/sg/SceneGraph.h"
#include "ospray/sg/geometry/TriangleMesh.h"
#include "ospray/sg/visitor/GatherNodesByName.h"
#include "ospray/sg/visitor/MarkAllAsModified.h"

#include "sg_utility/utility.h"

// custom visitors
#include "sg_visitors/GetVoxelRangeOfAllVolumes.h"
#include "sg_visitors/RecomputeBounds.h"
#include "sg_visitors/ReplaceAllTFs.h"

#include "widgets/MainWindow.h"

#include <ospray/ospcommon/utility/StringManip.h>

using namespace ospcommon;
using namespace ospray;

// Static data ////////////////////////////////////////////////////////////////

static int width       = 1200;
static int height      = 800;
static bool fullscreen = false;

static std::vector<std::string> filesToImport;
static std::vector<std::string> pluginsToLoad;

// Helper functions ///////////////////////////////////////////////////////////

static int initializeOSPRay(int *argc, const char *argv[])
{
  int init_error = ospInit(argc, argv);
  if (init_error != OSP_NO_ERROR) {
    std::cerr << "FATAL ERROR DURING INITIALIZATION!" << std::endl;
    return init_error;
  }

  auto device = ospGetCurrentDevice();
  if (device == nullptr) {
    std::cerr << "FATAL ERROR DURING GETTING CURRENT DEVICE!" << std::endl;
    return 1;
  }

  ospDeviceSetStatusFunc(device, [](const char *msg) { std::cout << msg; });
  ospDeviceSetErrorFunc(device, [](OSPError e, const char *msg) {
    std::cout << "OSPRAY ERROR [" << e << "]: " << msg << std::endl;
  });

  ospDeviceCommit(device);

  return 0;
}

static void parseCommandLine(int &ac, const char **&av)
{
  for (int i = 1; i < ac; i++) {
    const std::string arg = av[i];
    if (arg == "--fullscreen") {
      fullscreen = true;
      removeArgs(ac, av, i, 1);
      --i;
    } else if (arg == "-m" || arg == "--module") {
      ospLoadModule(av[i + 1]);
      removeArgs(ac, av, i, 2);
      --i;
    } else if (arg == "-w" || arg == "--width") {
      width = atoi(av[i + 1]);
      removeArgs(ac, av, i, 2);
      --i;
    } else if (arg == "-h" || arg == "--height") {
      height = atoi(av[i + 1]);
      removeArgs(ac, av, i, 2);
      --i;
    } else if (arg == "--size") {
      width  = atoi(av[i + 1]);
      height = atoi(av[i + 2]);
      removeArgs(ac, av, i, 3);
      --i;
    } else if (arg == "-sd") {
      width  = 640;
      height = 480;
    } else if (arg == "-hd") {
      width  = 1920;
      height = 1080;
    } else if (utility::beginsWith(arg, "-sg:")) {
      // SG parameters are validated by prefix only.
      // Later different function is used for parsing this type parameters.
      continue;
    } else if (arg == "--plugin" || arg == "-p") {
      pluginsToLoad.emplace_back(av[i + 1]);
      removeArgs(ac, av, i, 2);
      --i;
    } else {
      filesToImport.push_back(arg);
    }
  }
}

///////////////////////////////////////////////////////////////////////////////
// TODO: needs cleanup...
///////////////////////////////////////////////////////////////////////////////
void parseCommandLineSG(int ac, const char **&av, sg::Frame &root)
{
  for (int i = 1; i < ac; i++) {
    std::string arg(av[i]);
    size_t f;
    std::string value("");

    // Only parameters started with "-sg:" are allowed
    if (!utility::beginsWith(arg, "-sg:"))
      continue;

    // Store original argument before processing
    const std::string orgarg(arg);

    // Remove "-sg:" prefix
    arg = arg.substr(4, arg.length());

    while ((f = arg.find(":")) != std::string::npos ||
           (f = arg.find(",")) != std::string::npos) {
      arg[f] = ' ';
    }

    f = arg.find("+=");

    bool addNode = false;
    if (f != std::string::npos) {
      value   = arg.substr(f + 2, arg.size());
      addNode = true;
    } else {
      f = arg.find("=");
      if (f != std::string::npos)
        value = arg.substr(f + 1, arg.size());
    }
    if (value != "") {
      std::stringstream ss;
      ss << arg.substr(0, f);
      std::string child;
      std::reference_wrapper<sg::Node> node_ref = root;
      std::vector<std::shared_ptr<sg::Node>> children;
      while (ss >> child) {
        try {
          if (ss.eof())
            children = node_ref.get().childrenRecursive(child);
          else
            node_ref = node_ref.get().childRecursive(child);
        } catch (...) {
          std::cerr << "Warning: could not find child: " << child << std::endl;
        }
      }

      if (children.empty()) {
        std::cerr << "Warning: no children found for " << av[i] << " lookup\n";
        continue;
      }

      std::stringstream vals(value);

      if (addNode) {
        auto &node = *children[0];
        std::string name, type;
        vals >> name >> type;
        try {
          node.createChild(name, type);
        } catch (const std::runtime_error &) {
          std::cerr << "Warning: unknown sg::Node type '" << type
                    << "', ignoring option '" << orgarg << "'." << std::endl;
        }
      } else {  // set node value
        for (auto nodePtr : children) {
          auto &node = *nodePtr;
          // TODO: more generic implementation
          if (node.valueIsType<std::string>()) {
            node.setValue(value);
          } else if (node.valueIsType<float>()) {
            float x;
            vals >> x;
            node.setValue(x);
          } else if (node.valueIsType<int>()) {
            int x;
            vals >> x;
            node.setValue(x);
          } else if (node.valueIsType<bool>()) {
            bool x;
            vals >> x;
            node.setValue(x);
          } else if (node.valueIsType<ospcommon::vec3f>()) {
            vec3f v;
            vals >> v.x >> v.y >> v.z;
            node.setValue(v);
          } else if (node.valueIsType<ospcommon::vec3i>()) {
            vec3i v;
            vals >> v.x >> v.y >> v.z;
            node.setValue(v);
          } else if (node.valueIsType<ospcommon::vec2i>()) {
            vec2i v;
            vals >> v.x >> v.y;
            node.setValue(v);
          } else if (node.valueIsType<ospcommon::vec2f>()) {
            vec2f v;
            vals >> v.x >> v.y;
            node.setValue(v);
          } else {
            try {
              auto &vec = dynamic_cast<sg::DataVector1f &>(node);
              float f;
              while (vals.good()) {
                vals >> f;
                vec.push_back(f);
              }
            } catch (...) {
              std::cerr << "Cannot set value of node '" << node.name()
                        << "' on the command line!"
                        << " The expected value type is not (yet) handled."
                        << std::endl;
            }
          }
        }
      }
    }
  }
}

static void importFilesFromCommandLine(const sg::Frame &root)
{
  auto &renderer = root["renderer"];
  auto &world    = renderer["world"];

  for (auto file : filesToImport) {
    try {
      auto node = sg::createImporterNode(file);
      world.add(node);
    } catch (...) {
      std::cerr << "Failed to open file '" << file << "'!\n";
    }
  }

  /////////////////////////////////////////////////////////////////////////////
  // TODO: no! too many recomputing of scene bounds! ugh! fix this!
  /////////////////////////////////////////////////////////////////////////////
  if (!filesToImport.empty()) {
    renderer.traverse(sg::RecomputeBounds{});
    createDefaultView(root);
  }
}

static void setupLights(const sg::Frame &root)
{
  // TODO: this should be easy to add via the UI!
  auto &lights         = root.child("renderer").child("lights");
  auto &ambient        = lights.createChild("ambient", "AmbientLight");
  ambient["intensity"] = 1.25f;
  ambient["color"]     = vec3f(1.f);
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

int main(int argc, const char **argv)
{
  int result = initializeOSPRay(&argc, argv);

  if (result != 0) {
    std::cerr << "Failed to initialize OSPRay!" << std::endl;
    return result;
  }

  loadLibrary("ospray_sg");

  parseCommandLine(argc, argv);

  auto root = sg::createNode("ROOT", "Frame")->nodeAs<sg::Frame>();

  importFilesFromCommandLine(*root);
  setupLights(*root);

  MainWindow window(root, pluginsToLoad);

  auto master_tfn = window.getMasterTransferFunctioNode();
  root->traverse(sg::ReplaceAllTFs{master_tfn});

  root->commit();

  sg::GatherNodesByName getNodes("voxelRange");
  sg::GetVoxelRangeOfAllVolumes vrVisitor;
  root->traverse(vrVisitor);
  if (vrVisitor.numVoxelRangesFound > 0)
    master_tfn->child("valueRange") = vrVisitor.voxelRange.toVec2f();

  window.create("OSPRay Studio", fullscreen, vec2i(width, height));

  parseCommandLineSG(argc, argv, *root);

  imgui3D::run();

  return 0;
}
