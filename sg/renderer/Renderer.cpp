// Copyright 2009-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Renderer.h"
#include "sg/texture/Texture2D.h"

namespace ospray {
namespace sg {

Renderer::Renderer(std::string type)
{
  auto handle = ospNewRenderer(type.c_str());
  setHandle(handle);

  createChild("pixelSamples", "int", "samples-per-pixel", 1);
  createChild("maxPathLength", "int", "maximum ray recursion depth", 20);
  createChild("minContribution",
      "float",
      "minimum sample contribution, below which value is neglected",
      0.001f);
  createChild("varianceThreshold",
      "float",
      "stop rendering when variance < threshold",
      0.f);
  createChild("backgroundColor",
      "rgba",
      "transparent background color and alpha (RGBA), if no map_backplate set",
      rgba(vec3f(0.1f),1.f)); // Near black, with opaque alpha
  createChild("pixelFilter",
      "int",
      "pixel filter used by the renderer for antialiasing\n"\
      "(0=point, 1=box, 2=gauss, 3=mitchell, 4=blackman_harris)",
      (int)pixelFilter);

  child("pixelSamples").setMinMax(1, 1000);
  child("maxPathLength").setMinMax(0, 1000);
  child("minContribution").setMinMax(0.f, 10.f);
  child("varianceThreshold").setMinMax(0.f, 100.f);
  child("pixelFilter").setMinMax(0, 4);
}

NodeType Renderer::type() const
{
  return NodeType::RENDERER;
}

void Renderer::setNavMode(bool navMode)
{
  static auto oldNavMode = false;
  if (navMode != oldNavMode) {
    oldNavMode = navMode;

    // XXX TODO renderer separate navigation and still settings.
    // But, this is going to require more plumbing and UI to work correctly
    // and be usable.

    // Create 2 configurable sets of renderer options, to switch between.
    // Should work with any renderer type.
  }
}

// Register OSPRay's debug renderers //
struct OSPSG_INTERFACE DebugRenderer : public Renderer
{
  DebugRenderer();
  virtual ~DebugRenderer() override = default;
};

DebugRenderer::DebugRenderer() : Renderer("debug")
{
  createChild("method", "string", std::string("eyeLight"));
}

OSP_REGISTER_SG_NODE_NAME(DebugRenderer, renderer_debug);

} // namespace sg
} // namespace ospray
