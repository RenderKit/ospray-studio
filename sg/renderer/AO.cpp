// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Renderer.h"

namespace ospray {
namespace sg {

struct OSPSG_INTERFACE AORenderer : public Renderer
{
  AORenderer();
  virtual ~AORenderer() override = default;
  void preCommit() override;
};

OSP_REGISTER_SG_NODE_NAME(AORenderer, renderer_ao);

// AORenderer definitions /////////////////////////////////////////////////////

AORenderer::AORenderer() : Renderer("ao")
{
  createChild("aoSamples",
      "int",
      "number of rays per sample to compute ambient occlusion",
      1);
  createChild("aoDistance",
      "float",
      "maximum distance to consider for ambient occlusion",
      1e20f);
  createChild("aoIntensity", "float", "ambient occlusion strength", 1.f);
  createChild("volumeSamplingRate", "float", "sampling rate for volumes", 1.f);
}

void AORenderer::preCommit()
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
