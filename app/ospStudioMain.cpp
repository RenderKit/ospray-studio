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

#include "widgets/MainWindow.h"

using namespace ospcommon;

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

static int width  = 1200;
static int height = 800;
static bool fullscreen = false;

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
      width = atoi(av[i + 1]);
      height = atoi(av[i + 2]);
      removeArgs(ac, av, i, 3);
      --i;
    } else if (arg == "-sd") {
      width  = 640;
      height = 480;
    } else if (arg == "-hd") {
      width  = 1920;
      height = 1080;
    }
  }
}

int main(int argc, const char **argv)
{
  using namespace ospray;

  int result = initializeOSPRay(&argc, argv);

  if (result != 0) {
    std::cerr << "Failed to initialize OSPRay!" << std::endl;
    return result;
  }

  // access/load symbols/sg::Nodes dynamically
  loadLibrary("ospray_sg");

  parseCommandLine(argc, argv);

  auto root = sg::createNode("ROOT", "Frame")->nodeAs<sg::Frame>();

  // TODO: this should be easy to add via the UI! /////////////////////////////
  auto &lights = root->child("renderer").child("lights");
  auto &ambient = lights.createChild("ambient", "AmbientLight");
  ambient["intensity"] =  1.25f;
  ambient["color"] = vec3f(1.f);
  /////////////////////////////////////////////////////////////////////////////

  ospray::MainWindow window(root);

  window.create("OSPRay Studio", fullscreen, vec2i(width, height));

  imgui3D::run();

  return 0;
}
