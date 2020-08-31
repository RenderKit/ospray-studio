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

#include "Node.h"

namespace ospray {
  namespace sg {

  struct OSPSG_INTERFACE Frame : public OSPNode<cpp::Future, NodeType::FRAME>
  {
    Frame();
    ~Frame() override = default;

    NodeType type() const override;

    void startNewFrame();

    bool frameIsReady();
    float frameProgress();
    void waitOnFrame();
    void cancelFrame();
    bool accumLimitReached();

    bool denoiserEnabled{false};
    bool updateFrameOpsNextFrame{false};

    const void *mapFrame(OSPFrameBufferChannel = OSP_FB_COLOR);
    void unmapFrame(void *mem);
    void saveFrame(std::string filename, int flags);

    bool immediatelyWait{false};
    bool pauseRendering{false};
    int accumLimit{0};
    int currentAccum{0};

   private:
    void refreshFrameOperations();
    void preCommit() override;
    void postCommit() override;
  };

  }  // namespace sg
} // namespace ospray
