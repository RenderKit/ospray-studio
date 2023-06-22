// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Renderer.h"

namespace ospray {
namespace sg {

struct OSPSG_INTERFACE PathTracer : public Renderer
{
  PathTracer();
  virtual ~PathTracer() override = default;
};

OSP_REGISTER_SG_NODE_NAME(PathTracer, renderer_pathtracer);

// PathTracer definitions ///////////////////////////////////////////////////

PathTracer::PathTracer() : Renderer("pathtracer")
{
  createChild("lightSamples",
      "int",
      "number of random light samples per path vertex",
      -1);
  createChild("maxScatteringEvents",
      "int",
      "maximum number of non-specular (glossy and diffuse) bounces",
      20);
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
  createChild("backgroundRefraction",
      "bool",
      "allow for alpha blending even if background is seen through refractive objects like glass",
      false);
  createChild("shadowCatcherPlane",
      "vec4f",
      "shadow catcher plane offset and normal (all zeros disable)",
      vec4f(0));

  child("maxScatteringEvents").setMinMax(1, 100);
  child("lightSamples").setMinMax(-1, 1000);
  child("roulettePathLength").setMinMax(0, 1000);
  child("maxContribution").setMinMax(0.f, 1e6f);
}

} // namespace sg
} // namespace ospray
