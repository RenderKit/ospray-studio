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
// TODO: this is replicated code from MainWindow::resetDefaultView()...!!!
///////////////////////////////////////////////////////////////////////////////
static void createDefaultView(const sg::Frame &root)
{
  auto &world = root["renderer"]["world"];
  auto bbox   = world.bounds();
  vec3f diag  = bbox.size();
  diag        = max(diag, vec3f(0.3f * length(diag)));

  auto gaze = ospcommon::center(bbox);
  auto pos  = gaze - .75f * vec3f(-.6 * diag.x, -1.2f * diag.y, .8f * diag.z);
  auto up   = vec3f(0.f, 1.f, 0.f);

  auto &camera  = root["camera"];
  camera["pos"] = pos;
  camera["dir"] = normalize(gaze - pos);
  camera["up"]  = up;
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

  imgui3D::run();

  return 0;
}
