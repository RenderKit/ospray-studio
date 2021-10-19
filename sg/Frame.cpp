// Copyright 2009-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Frame.h"

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

void Frame::startNewFrame(bool interacting)
{
  auto &fb = childAs<FrameBuffer>("framebuffer");
  auto &camera = childAs<Camera>("camera");
  auto &renderer = childAs<Renderer>("renderer");
  auto &world = childAs<World>("world");

  // If working on a frame, cancel it, something has changed
  if (isModified()) {
    cancelFrame();
    waitOnFrame();
    resetAccumulation();
    // Enable navMode
    if (!navMode)
      child("navMode") = true;
  } else {
    // No frame changes, disable navMode
    if (navMode)
      child("navMode") = false;
  }

  refreshFrameOperations();

  // Commit only when modified and not while interacting.
  if (isModified() && !interacting)
    commit();

  if (!(interacting || pauseRendering || accumLimitReached())) {
    auto future = fb.handle().renderFrame(
        renderer.handle(), camera.handle(), world.handle());
    setHandle(future);
    commit(); // XXX setHandle modifies node, but nothing else has changed yet
    canceled = false;

    if (immediatelyWait)
      waitOnFrame();
    if (!accumLimitReached())
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
  return (accumLimit > 0 && currentAccum == accumLimit - 1);
}

void Frame::resetAccumulation()
{
  auto &fb = childAs<FrameBuffer>("framebuffer");
  fb.resetAccumulation();
  currentAccum = 0;
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
  auto denoiserEnabled = navMode ? denoiseNavFB : denoiseFB;
  auto toneMapperEnabled = navMode ? toneMapNavFB : toneMapFB;

  if (denoiserEnabled && denoiseFBFinalFrame && accumAtFinal())
    denoiserEnabled = true;
  else
    denoiserEnabled = false;

  fb.updateDenoiser(denoiserEnabled);
  fb.updateToneMapper(toneMapperEnabled);
  fb.updateImageOperations();
}

void Frame::preCommit()
{
  static bool currentNavMode = navMode;
  navMode = child("navMode").valueAs<bool>();

  if (navMode != currentNavMode) {
    currentNavMode = navMode;

    // Recreate framebuffers on windowsize or scale changes.
    auto &fb = child("framebuffer");
    auto oldSize = fb["size"].valueAs<vec2i>();
    auto scale = (navMode ? child("scaleNav").valueAs<float>()
                          : child("scale").valueAs<float>());

    auto newSize = (vec2i)(child("windowSize").valueAs<vec2i>() * scale);
    if (oldSize != newSize)
      fb["size"] = newSize;
  }

  // The materials list needs to know of any change in renderer type.
  // Yet, the renderer has no direct way of notifying the material registery,
  // so update the rendererType here.
  auto &renderer = childAs<Renderer>("renderer");
  if (renderer.isModified())
    baseMaterialRegistry->updateRendererType();
}

void Frame::postCommit() {}

OSP_REGISTER_SG_NODE_NAME(Frame, frame);

} // namespace sg
} // namespace ospray
