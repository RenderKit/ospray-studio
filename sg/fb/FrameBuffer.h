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

#pragma once

#include "sg/Node.h"

namespace ospray::sg {

  struct OSPSG_INTERFACE FrameBuffer
      : public OSPNode<cpp::FrameBuffer, NodeType::FRAME_BUFFER>
  {
    FrameBuffer();
    ~FrameBuffer() override = default;

    NodeType type() const override;

    const void *map(OSPFrameBufferChannel = OSP_FB_COLOR);
    void unmap(const void *mem);

    void resetAccumulation();

   private:
    void postCommit() override;

    void updateHandle();

    std::map<std::string, OSPFrameBufferFormat> colorFormats{
        {"sRGB", OSP_FB_SRGBA},
        {"RGBA8", OSP_FB_RGBA8},
        {"float", OSP_FB_RGBA32F},
        {"none", OSP_FB_NONE}};
  };

}  // namespace ospray::sg
