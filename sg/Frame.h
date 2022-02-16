// Copyright 2009-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

// ospray sg
#include "Node.h"
#include "camera/Camera.h"
#include "fb/FrameBuffer.h"
#include "renderer/MaterialRegistry.h"
#include "renderer/Renderer.h"
#include "scene/World.h"
#include "scene/lights/LightsManager.h"

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
    bool accumAtFinal();
    void resetAccumulation();

    inline bool isCanceled()
    {
      return canceled;
    }

    void setNavMode(bool mode)
    {
      navMode = mode;
    }

    bool denoiserEnabled{false};
    bool denoiseFB{false};
    bool denoiseNavFB{false};
    bool denoiseFBFinalFrame{false};
    bool denoiseOnlyPathTracer{true};

    bool toneMapFB{false};
    bool toneMapNavFB{false};

    const void *mapFrame(OSPFrameBufferChannel = OSP_FB_COLOR);
    void unmapFrame(void *mem);
    void saveFrame(std::string filename, int flags);

    bool immediatelyWait{false};
    bool pauseRendering{false};
    int accumLimit{0};
    int currentAccum{0};
    bool canceled{false};

    // Shared by all frames
    std::shared_ptr<sg::MaterialRegistry> baseMaterialRegistry;
    std::shared_ptr<sg::LightsManager> lightsManager;

   private:
    bool navMode{false};
    void refreshFrameOperations();
    void preCommit() override;
    void postCommit() override;
  };

  }  // namespace sg
} // namespace ospray
