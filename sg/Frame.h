// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

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
    bool denoiseFB{false};
    bool denoiseNavFB{false};
    
    const void *mapFrame(OSPFrameBufferChannel = OSP_FB_COLOR);
    void unmapFrame(void *mem);
    void saveFrame(std::string filename, int flags);

    bool immediatelyWait{false};
    bool pauseRendering{false};
    int accumLimit{0};
    int currentAccum{0};

   private:
    bool navMode{false};
    void refreshFrameOperations();
    void preCommit() override;
    void postCommit() override;
  };

  }  // namespace sg
} // namespace ospray
