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

#include "Frame.h"

#include "sg/camera/Camera.h"
#include "sg/fb/FrameBuffer.h"
#include "sg/renderer/Renderer.h"
#include "sg/scene/World.h"
#include "sg/scene/lights/Lights.h"

namespace ospray::sg {

  Frame::Frame()
  {
    createChild("windowSize", "vec2i", vec2i(1024, 768));
    createChild("framebuffer", "framebuffer");
    createChild("scaleFB", "float", 1.f);
    createChild("denoiseFB", "bool", false);
    createChild("camera", "camera_perspective");
    createChild("renderer", "renderer_scivis");
    createChild("world", "world");

    child("windowSize").setReadOnly();
    child("scaleFB").setReadOnly();
  }

  NodeType Frame::type() const
  {
    return NodeType::FRAME;
  }

  void Frame::startNewFrame()
  {
    auto &fb       = childAs<FrameBuffer>("frameBuffer");
    auto &camera   = childAs<Camera>("camera");
    auto &renderer = childAs<Renderer>("renderer");
    auto &world    = childAs<World>("world");

    refreshFrameOperations();

    if (this->anyChildModified())
      fb.resetAccumulation();

    this->commit();
    if (!pauseRendering) {
      auto future = fb.handle().renderFrame(
          renderer.handle(), camera.handle(), world.handle());
      setHandle(future);

      if (immediatelyWait)
        waitOnFrame();
    }
  }

  bool Frame::frameIsReady()
  {
    auto future = handle();
    if (future)
      return future.isReady();
    else
      return false;
  }

  float Frame::frameProgress()
  {
    auto future = handle();
    if (future)
      return future.progress();
    else
      return 1.f;
  }

  void Frame::waitOnFrame()
  {
    auto future = handle();
    if (future)
      future.wait();
  }

  void Frame::cancelFrame()
  {
    auto future = handle();
    if (future)
      future.cancel();
  }

  const void *Frame::mapFrame(OSPFrameBufferChannel channel)
  {
    waitOnFrame();
    auto &fb = childAs<FrameBuffer>("framebuffer");
    return fb.map(channel);
  }

  void Frame::unmapFrame(void *mem)
  {
    auto &fb = childAs<FrameBuffer>("framebuffer");
    fb.unmap(mem);
  }

  void Frame::saveFrame(std::string filename, int flags)
  {
    auto &fb = childAs<FrameBuffer>("framebuffer");
    fb.saveFrame(filename, flags);
  }

  void Frame::refreshFrameOperations()
  {
    if (updateFrameOpsNextFrame) {
      auto &fb = childAs<FrameBuffer>("framebuffer");
      fb.updateDenoiser(denoiserEnabled);
      updateFrameOpsNextFrame = false;
    }
  }

  void Frame::preCommit()
  {
    // Update frameOps on a change in denoiser value
    auto newDenoiseValue = child("denoiseFB").valueAs<bool>();
    updateFrameOpsNextFrame = denoiserEnabled ^ newDenoiseValue;
    denoiserEnabled = newDenoiseValue;

    // Recreate framebuffers on windowsize or scale changes.
    auto &fb     = child("framebuffer");
    auto oldSize = fb["size"].valueAs<vec2i>();
    auto newSize = (vec2i)(child("windowSize").valueAs<vec2i>() *
                           child("scaleFB").valueAs<float>());
    if (oldSize != newSize) {
      fb["size"] = newSize; 
      // These will override a change to the denoise child state.
      updateFrameOpsNextFrame = true;
      denoiserEnabled = false;
    }

    if (hasChild("lights")) {
      auto &lights = childAs<sg::Lights>("lights");
      std::vector<cpp::Light> lightObjects;

      for (auto &name : lights.lightNames) {
        auto &l = lights.child(name).valueAs<cpp::Light>();
        lightObjects.push_back(l);
      }
      auto &ospWorld = childAs<World>("world").valueAs<cpp::World>();

      ospWorld.setParam("light", cpp::CopiedData(lightObjects));
    }
  }

  void Frame::postCommit() {
    auto ospWorld = childAs<World>("world").valueAs<cpp::World>();
    ospWorld.commit();
  }

  OSP_REGISTER_SG_NODE_NAME(Frame, frame);

}  // namespace ospray::sg
