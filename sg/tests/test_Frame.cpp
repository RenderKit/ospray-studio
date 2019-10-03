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

#include "sg/Frame.h"
using namespace ospray::sg;

#include "ospcommon/utility/SaveImage.h"

int main(int argc, const char *argv[])
{
  ospInit(&argc, argv);

  std::cout << "Rendering a test frame..." << std::endl;

  auto frame_ptr = createNodeAs<Frame>("frame", "Frame", "test frame");
  auto &frame    = *frame_ptr;

  frame.startNewFrame();
  frame.waitOnFrame();

  std::cout << "...finished!" << std::endl;

  auto size    = frame["frameBuffer"]["size"].valueAs<vec2i>();
  auto *pixels = (uint32_t *)frame.mapFrame();

  ospcommon::utility::writePPM("test_Frame.ppm", size.x, size.y, pixels);

  frame.unmapFrame(pixels);

  std::cout << "\nresult saved to 'test_Frame.ppm'" << std::endl;

  frame_ptr.reset();
  ospShutdown();

  return 0;
}
