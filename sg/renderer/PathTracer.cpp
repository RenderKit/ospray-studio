// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Renderer.h"

namespace ospray {
namespace sg {

struct OSPSG_INTERFACE PathTracer : public Renderer
{
  PathTracer();
  virtual ~PathTracer() override = default;
  void preCommit() override;
};

OSP_REGISTER_SG_NODE_NAME(PathTracer, renderer_pathtracer);

// PathTracer definitions ///////////////////////////////////////////////////

PathTracer::PathTracer() : Renderer("pathtracer")
{
  createChild("lightSamples",
      "int",
      "number of random light samples per path vertex",
      -1);
  createChild("roulettePathLength",
      "int",
      "ray recursion depth at which to start roulette termination",
      5);
  createChild("maxContribution",
      "float",
      "clamped value for samples accumulated into the framebuffer",
      1e6f);
  createChild("geometryLights",
      "bool",
      "whether geometries with an emissive material illuminate the scene",
      true);
}

void PathTracer::preCommit()
{
  this->Renderer::preCommit();

  auto &renderer = handle();
  if (hasChild("map_backplate")) {
    auto &cppTex = child("map_backplate").valueAs<cpp::Texture>();
    renderer.setParam("map_backplate", cppTex);
  }
}

} // namespace sg
} // namespace ospray
