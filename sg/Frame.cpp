// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Frame.h"
#include "FileWatcher.h"

namespace ospray {
namespace sg {

Frame::Frame()
{
  createChild("windowSize", "vec2i", vec2i(1024, 768));
  createChild("framebuffer", "framebuffer");
  createChild("scale", "float", 1.f);
  createChild("scaleNav", "float", 0.5f);
  createChild("camera", "camera_perspective");
  createChild("renderer", "renderer_pathtracer");
  createChild("world", "world");
  createChild("navMode", "bool", false);

  child("windowSize").setReadOnly();
  child("scale").setReadOnly();
  child("scaleNav").setReadOnly();
  child("navMode").setReadOnly();

  // Shared by all frames
  baseMaterialRegistry = sg::createNodeAs<sg::MaterialRegistry>(
      "baseMaterialRegistry", "materialRegistry");
  lightsManager = sg::createNodeAs<sg::LightsManager>("lights", "lights");
  add(baseMaterialRegistry);
  add(lightsManager);
}

NodeType Frame::type() const
{
  return NodeType::FRAME;
}

void Frame::startNewFrame()
{
  auto &fb = childAs<FrameBuffer>("framebuffer");
  auto &camera = childAs<Camera>("camera");
  auto &renderer = childAs<Renderer>("renderer");
  auto &world = childAs<World>("world");

  if (fb["targetFrames"].valueAs<int>() != accumLimit)
    fb["targetFrames"] = accumLimit;

  // App navMode set via setNavMode() vs current frame-state navMode
  if (navMode != child("navMode").valueAs<bool>())
    child("navMode") = navMode && (!navMode || isModified());

  // This will update all nodes that are watching for file changes
  checkFileWatcherModifications();

  // If working on a frame, cancel it, something has changed
  if (isModified()) {
    cancelFrame();
    waitOnFrame();
    resetAccumulation();
  }

  refreshFrameOperations();

  // Commit only when modified
  if (isModified())
    commit();

  if (!(pauseRendering || accumLimitReached() || varThresholdReached())) {
    auto future = fb.handle().renderFrame(
        renderer.handle(), camera.handle(), world.handle());
    setHandle(future, false); // setHandle but don't update modified time
    canceled = false;

    if (immediatelyWait)
      waitOnFrame();
    if (!accumLimitReached() && !varThresholdReached())
      currentAccum++;
  }
}

bool Frame::frameIsReady()
{
  auto future = value().valid() ? handle() : nullptr;
  if (future)
    return future.isReady(OSP_FRAME_FINISHED);
  else
    return false;
}

float Frame::frameProgress()
{
  auto future = value().valid() ? handle() : nullptr;
  if (future)
    return future.progress();
  else
    return 1.f;
}

float Frame::frameDuration()
{
  auto future = value().valid() ? handle() : nullptr;
  if (future)
    return future.duration();
  else
    return -1.f;
}

void Frame::waitOnFrame()
{
  auto future = value().valid() ? handle() : nullptr;
  if (future)
    future.wait(OSP_FRAME_FINISHED);
}

void Frame::cancelFrame()
{
  auto future = value().valid() ? handle() : nullptr;
  if (future)
    future.cancel();
  canceled = true;
}

bool Frame::accumLimitReached()
{
  return (accumLimit > 0 && currentAccum >= accumLimit);
}

bool Frame::accumAtFinal()
{
  return (accumLimit > 0 && currentAccum >= accumLimit - 1);
}

void Frame::resetAccumulation()
{
  auto &fb = childAs<FrameBuffer>("framebuffer");
  fb.resetAccumulation();
  currentAccum = 0;
}

bool Frame::varThresholdReached()
{
  auto &fb = childAs<FrameBuffer>("framebuffer");
  auto &renderer = childAs<Renderer>("renderer");
  auto varianceThreshold = renderer["varianceThreshold"].valueAs<float>();
  return (varianceThreshold > 0 && fb.variance() > 0 && fb.variance() <= varianceThreshold);
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
  auto &fb = childAs<FrameBuffer>("framebuffer");
  auto &vt = childAs<Renderer>("renderer")["varianceThreshold"];
  auto denoiserEnabled = navMode ? denoiseNavFB : denoiseFB;
  auto toneMapperChanged =
      fb.getToneMapper() ? fb.getToneMapper()->isModified() : false;
  auto denoiserChanged =
      fb.getDenoiser() ? fb.getDenoiser()->isModified() : false;

  denoiserEnabled &=
      (!denoiseFBFinalFrame || (denoiseFBFinalFrame && (accumAtFinal()
          || (fb.variance() < vt.valueAs<float>()))));

  denoiserEnabled &= !(denoiseOnlyPathTracer
      && (child("renderer")["type"].valueAs<std::string>() != "pathtracer"));

  fb.updateDenoiser(denoiserEnabled, accumAtFinal());
  fb.updateToneMapper(toneMapFB);
  fb.updateImageOperations();

  uint8_t newFrameOpsState = denoiserEnabled << 1 | toneMapFB;
  static uint8_t lastFrameOpsState{0};

  // If there's a change and accumLimit is already reached, force another frame
  // so operations will occur
  if ((newFrameOpsState != lastFrameOpsState || denoiserChanged
          || toneMapperChanged)
      && accumLimitReached())
    currentAccum--;

  lastFrameOpsState = newFrameOpsState;
}

void Frame::preCommit()
{
  // Recreate framebuffers on windowsize or scale changes.
  auto &fb = child("framebuffer");
  auto oldSize = fb["size"].valueAs<vec2i>();
  auto scale = (navMode ? child("scaleNav").valueAs<float>()
      : child("scale").valueAs<float>());

  auto newSize = (vec2i)(child("windowSize").valueAs<vec2i>() * scale);
  if (oldSize != newSize)
    fb["size"] = newSize;

  // The materials list needs to know of any change in renderer type.
  // Yet, the renderer has no direct way of notifying the material registry,
  // so update the rendererType here.
  auto &renderer = childAs<Renderer>("renderer");
  if (renderer.isModified())
    baseMaterialRegistry->updateRendererType();
}

void Frame::postCommit() {}

OSP_REGISTER_SG_NODE_NAME(Frame, frame);

} // namespace sg
} // namespace ospray
