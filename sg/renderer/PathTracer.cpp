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
    createChild("maxPathLength", "int", 100);
    createChild("roulettePathLength", "int", 10000);
    createChild("geometryLights", "bool", true);
    createChild("lightSamples", "int", -1);
    createChild("maxContribution", "float", (float)1e6);
  }

  void PathTracer::preCommit() {
    this->Renderer::preCommit();

    auto &renderer = handle();
    if(hasChild("map_backplate")) {
      auto &cppTex = child("map_backplate").valueAs<cpp::Texture>();
      renderer.setParam("map_backplate", cppTex);
    }
  }

  }  // namespace sg
} // namespace ospray
