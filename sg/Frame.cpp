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

namespace ospray::sg {

  Frame::Frame()
  {
    createChild("framebuffer", "framebuffer");
    createChild("camera", "camera_perspective");
    createChild("renderer", "renderer_scivis");
    createChild("world", "world");
  }

  NodeType Frame::type() const
  {
    return NodeType::FRAME;
  }

  void Frame::startNewFrame(bool immediatelyWait)
  {
    auto &fb       = childAs<FrameBuffer>("frameBuffer");
    auto &camera   = childAs<Camera>("camera");
    auto &renderer = childAs<Renderer>("renderer");
    auto &world    = childAs<World>("world");

    if (updateFrameOpsNextFrame) {
      refreshFrameOperations();
      updateFrameOpsNextFrame = false;
    }

    if (this->anyChildModified())
      fb.resetAccumulation();

    this->commit();

    auto future = fb.handle().renderFrame(
        renderer.handle(), camera.handle(), world.handle());
    setHandle(future);

    if (immediatelyWait)
      waitOnFrame();
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

  void Frame::saveFrame(std::string filename)
  {
    auto &fb = childAs<FrameBuffer>("framebuffer");
    fb.saveFrame(filename);
  }

  void Frame::refreshFrameOperations()
  {
    auto &fb = childAs<FrameBuffer>("framebuffer");
    fb.updateDenoiser(denoiserEnabled);
  }

  void Frame::preCommit() {}

  void Frame::postCommit() {}

  OSP_REGISTER_SG_NODE_NAME(Frame, frame);

}  // namespace ospray::sg
