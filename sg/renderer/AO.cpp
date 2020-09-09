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

  // AORenderer definitions ///////////////////////////////////////////////////////

  AORenderer::AORenderer() : Renderer("ao")
  {
    createChild("aoSamples", "int", 1);
    createChild("aoRadius", "float", 1e20f);
    createChild("aoIntensity", "float", 1.f);
    createChild("volumeSamplingRate", "float", 1.f);
  }

  void AORenderer::preCommit() {
    this->Renderer::preCommit();

    auto &renderer = handle();
    if(hasChild("map_backplate")) {
      auto &cppTex = child("map_backplate").valueAs<cpp::Texture>();
      renderer.setParam("map_backplate", cppTex);
    }
  }

  }  // namespace sg
} // namespace ospray
