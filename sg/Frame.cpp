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

namespace ospray {
  namespace sg {

    Frame::Frame()
    {
      createChild("frameBuffer", "FrameBuffer", "current FrameBuffer");
      createChild("camera", "PerspectiveCamera", "current Camera");
      createChild("renderer", "Renderer_rayDir", "current Renderer");
      createChild("world", "World", "current World");
    }

    void Frame::startNewFrame()
    {
      auto future = handle();
      if (future) {
        ospWait(future);
        ospRelease(future);
      }

      this->commit();

      auto &fb       = childAs<FrameBuffer>("frameBuffer");
      auto &camera   = childAs<Camera>("camera");
      auto &renderer = childAs<Renderer>("renderer");
      auto &world    = childAs<World>("world");

      future = ospRenderFrame(
          fb.handle(), renderer.handle(), camera.handle(), world.handle());
      setHandle(future);
    }

    bool Frame::frameIsReady()
    {
      auto future = handle();
      if (future)
        return ospIsReady(future);
      else
        return false;
    }

    float Frame::frameProgress()
    {
      auto future = handle();
      if (future)
        return ospGetProgress(future);
      else
        return 1.f;
    }

    void Frame::waitOnFrame()
    {
      auto future = handle();
      if (future)
        ospWait(future);
    }

    void Frame::cancelFrame()
    {
      auto future = handle();
      if (future)
        ospCancel(future);
    }

    const void *Frame::mapFrame(OSPFrameBufferChannel channel)
    {
      waitOnFrame();
      auto &fb = childAs<FrameBuffer>("frameBuffer");
      return fb.map(channel);
    }

    void Frame::unmapFrame(void *mem)
    {
      auto &fb = childAs<FrameBuffer>("frameBuffer");
      fb.unmap(mem);
    }

    void Frame::preCommit() {}

    void Frame::postCommit() {}

    OSP_REGISTER_SG_NODE(Frame);

  }  // namespace sg
}  // namespace ospray
