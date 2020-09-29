// Copyright 2009-2020 Intel Corporation
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
      rgba(0.1f));
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
  std::cout << "XXX TODO renderer setNavMode " << (navMode ? "on" : "off") << std::endl;
  // But, refreshRenderer creates a new child that will blow away current unless
  // hanging off the frame or something?!
  //
  // Create and *enable nav renderer* setting that creates/enables the nav renderer
  //
  // XXX Create 2 configurable sets of render options, 2 renderers to switch
  // between! and, should have 2 of *each type* of renderer, not just PT This
  // should move into renderer.cpp
#if 0 // Just iterate through children copying each one to active renderer
  child("maxPathLength")              = navMode ? 12 : 32;
  child("densityFadeDepth")           = navMode ? 1 : 18;
  child("densityFade")                = navMode ? 0.05f : 0.1f;
  child("transmittanceDistanceScale") = navMode ? 6.f : 6.f;
  child("albedoScale")                = navMode ? 1.1f : 1.03f;
  child("densityScale")               = navMode ? 1.f : 1.f;
#endif
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
