// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include <iostream>

#include "sg/Data.h"
#include "sg/Frame.h"
#include "sg/visitors/PrintNodes.h"
using namespace ospray::sg;

#include "rkcommon/utility/SaveImage.h"

int main(int argc, const char *argv[])
{
  auto initError = ospInit(&argc, argv);

  if (initError != OSP_NO_ERROR)
    throw std::runtime_error("OSPRay not initialized correctly!");

  OSPDevice device = ospGetCurrentDevice();
  if (!device)
    throw std::runtime_error("OSPRay device could not be fetched!");

  ospDeviceSetErrorCallback(
      device,
      [](void *, OSPError, const char *errorDetails) {
        std::cerr << "OSPRay error: " << errorDetails << std::endl;
      },
      nullptr);

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

    auto frame_ptr = createNodeAs<Frame>("frame", "frame");
    auto &frame    = *frame_ptr;

    frame.createChild("renderer", "renderer_scivis");

    frame["camera"]["aspect"]    = imgSize.x / (float)imgSize.y;
    frame["camera"]["position"]  = cam_pos;
    frame["camera"]["direction"] = cam_view;
    frame["camera"]["up"]        = cam_up;

    auto xfm = createNode("xfm", "transform", affine3f::translate(vec3f(0.1f)));

    // create and setup model and mesh
    auto mesh = createNode("mesh", "geometry_triangles");

    mesh->createChildData("vertex.position", vertex);
    mesh->createChildData("vertex.color", color);
    mesh->createChildData("index", index);

    xfm->add(mesh);

    frame["world"].add(xfm);

    frame.render();

    for (int i = 0; i < 10; ++i) {
      frame.immediatelyWait = true;
      frame.startNewFrame();
    }

    std::cout << "...finished!" << std::endl;

    auto size    = frame["framebuffer"]["size"].valueAs<vec2i>();
    auto *pixels = (uint32_t *)frame.mapFrame();

    rkcommon::utility::writePPM("test_sgTutorial.ppm", size.x, size.y, pixels);

    frame.unmapFrame(pixels);

    std::cout << "\nresult saved to 'test_sgTutorial.ppm'" << std::endl;

    std::cout << "\n**** world bounds **** \n\n";

    std::cout << frame["world"].bounds() << std::endl;

    std::cout << "\n**** tree structure **** \n\n";
    frame.traverse<PrintNodes>();
  }

  ospShutdown();

  return 0;
}
