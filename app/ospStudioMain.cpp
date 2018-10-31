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

#include "sg/SceneGraph.h"
#include "sg/geometry/TriangleMesh.h"
#include "sg/visitor/MarkAllAsModified.h"

#include "sg_utility/utility.h"
#include "sg_visitors/RecomputeBounds.h"
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
    if (arg.compare(0, 4, "-sg:") != 0)
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
            float x, y, z;
            vals >> x >> y >> z;
            node.setValue(ospcommon::vec3f(x, y, z));
          } else if (node.valueIsType<ospcommon::vec3i>()) {
            int x, y, z;
            vals >> x >> y >> z;
            node.setValue(ospcommon::vec3i(x, y, z));
          } else if (node.valueIsType<ospcommon::vec2i>()) {
            int x, y;
            vals >> x >> y;
            node.setValue(ospcommon::vec2i(x, y));
          } else if (node.valueIsType<ospcommon::vec2f>()) {
            float x, y;
            vals >> x >> y;
            node.setValue(ospcommon::vec2f(x, y));
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
      FileName fn = file;
      std::stringstream ss;
      ss << fn.name();
      auto importerNode_ptr = sg::createNode(ss.str(), "Importer");
      auto &importerNode    = *importerNode_ptr;

      importerNode["fileName"] = fn.str();

      if (importerNode.hasChildRecursive("gradientShadingEnabled"))
        importerNode.childRecursive("gradientShadingEnabled") = false;
      if (importerNode.hasChildRecursive("adaptiveMaxSamplingRate"))
        importerNode.childRecursive("adaptiveMaxSamplingRate") = 0.2f;

      auto &transform = world.createChild("transform_" + ss.str(), "Transform");
      transform.add(importerNode_ptr);

      importerNode.traverse(sg::RecomputeBounds{});

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

    renderer.verify();
    renderer.commit();
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

  window.create("OSPRay Studio", fullscreen, vec2i(width, height));

  parseCommandLineSG(argc, argv, *root);

  imgui3D::run();

  return 0;
}
