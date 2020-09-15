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
      "pixel filter used by the renderer for antialiasing",
      (int)pixelFilter);
}

NodeType Renderer::type() const
{
  return NodeType::RENDERER;
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
