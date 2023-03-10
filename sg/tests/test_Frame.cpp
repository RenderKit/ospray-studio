// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include <iostream>

#include "sg/Frame.h"
#include "sg/visitors/PrintNodes.h"
using namespace ospray::sg;
using namespace rkcommon::math;

#include "rkcommon/utility/SaveImage.h"

int main(int argc, const char *argv[])
{
  ospInit(&argc, argv);

  {
    std::cout << "Rendering a test frame..." << std::endl;

    auto frame_ptr = createNodeAs<Frame>("frame", "frame");
    auto &frame    = *frame_ptr;

    frame.startNewFrame();
    frame.waitOnFrame();

    std::cout << "...finished!" << std::endl;

    auto size    = frame["framebuffer"]["size"].valueAs<vec2i>();
    auto *pixels = (uint32_t *)frame.mapFrame();

    rkcommon::utility::writePPM("test_Frame.ppm", size.x, size.y, pixels);

    frame.unmapFrame(pixels);

    std::cout << "\nresult saved to 'test_Frame.ppm'" << std::endl;

    std::cout << "\n**** tree structure **** \n\n";
    frame.traverse<PrintNodes>();
  }

  ospShutdown();

  return 0;
}
