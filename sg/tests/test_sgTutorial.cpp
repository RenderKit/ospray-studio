// ======================================================================== //
// Copyright 2009-2019 Intel Corporation                                    //
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

#include <iostream>

#include "sg/Data.h"
#include "sg/Frame.h"
#include "sg/visitors/PrintNodes.h"
using namespace ospray::sg;

#include "ospcommon/utility/SaveImage.h"

int main(int argc, const char *argv[])
{
  auto initError = ospInit(&argc, argv);

  if (initError != OSP_NO_ERROR)
    throw std::runtime_error("OSPRay not initialized correctly!");

  OSPDevice device = ospGetCurrentDevice();
  if (!device)
    throw std::runtime_error("OSPRay device could not be fetched!");

  ospDeviceSetErrorFunc(device, [](OSPError error, const char *errorDetails) {
    std::cerr << "OSPRay error: " << errorDetails << std::endl;
    exit(error);
  });

  // image size
  vec2i imgSize(1024, 768);

  // camera
  vec3f cam_pos{0.f, 0.f, 0.f};
  vec3f cam_up{0.f, 1.f, 0.f};
  vec3f cam_view{0.1f, 0.f, 1.f};

  // triangle mesh data
  std::vector<vec3f> vertex = {vec3f(-1.0f, -1.0f, 3.0f),
                               vec3f(-1.0f, 1.0f, 3.0f),
                               vec3f(1.0f, -1.0f, 3.0f),
                               vec3f(0.1f, 0.1f, 0.3f)};

  std::vector<vec4f> color = {vec4f(0.9f, 0.5f, 0.5f, 1.0f),
                              vec4f(0.8f, 0.8f, 0.8f, 1.0f),
                              vec4f(0.8f, 0.8f, 0.8f, 1.0f),
                              vec4f(0.5f, 0.9f, 0.5f, 1.0f)};

  std::vector<vec3ui> index = {vec3ui(0, 1, 2), vec3ui(1, 2, 3)};

  {
    std::cout << "Rendering a test frame..." << std::endl;

    auto frame_ptr = createNodeAs<Frame>("frame", "Frame", "test frame");
    auto &frame    = *frame_ptr;

    frame.createChild("renderer", "Renderer_raycast", "current Renderer");

    frame["camera"]["aspect"]    = imgSize.x / (float)imgSize.y;
    frame["camera"]["position"]  = cam_pos;
    frame["camera"]["direction"] = cam_view;
    frame["camera"]["up"]        = cam_up;

    // create and setup model and mesh
    auto mesh = createNode("mesh", "Triangles", "triangle mesh");

    mesh->createChildData("vertex.position", vertex);
    mesh->createChildData("vertex.color", color);
    mesh->createChildData("index", index);

    frame["world"].add(mesh);

    frame.commit();
    frame["world"].render();

    frame.startNewFrame();
    frame.waitOnFrame();

    std::cout << "...finished!" << std::endl;

    auto size    = frame["frameBuffer"]["size"].valueAs<vec2i>();
    auto *pixels = (uint32_t *)frame.mapFrame();

    ospcommon::utility::writePPM("test_sgTutorial.ppm", size.x, size.y, pixels);

    frame.unmapFrame(pixels);

    std::cout << "\nresult saved to 'test_sgTutorial.ppm'" << std::endl;

    std::cout << "\n**** tree structure **** \n\n";
    frame.traverse<PrintNodes>();
  }

  ospShutdown();

  return 0;
}
