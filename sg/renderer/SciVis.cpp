// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Renderer.h"

namespace ospray {
  namespace sg {

  struct OSPSG_INTERFACE SciVis : public Renderer
  {
    SciVis();
    virtual ~SciVis() override = default;
    void preCommit() override;
  };

  OSP_REGISTER_SG_NODE_NAME(SciVis, renderer_scivis);

  // SciVis definitions ///////////////////////////////////////////////////////

  SciVis::SciVis() : Renderer("scivis")
  {
    createChild("aoSamples", "int", 1);
    createChild("aoRadius", "float", 1e20f);
    createChild("aoIntensity", "float", 1.f);
  }

  void SciVis::preCommit() {
    this->Renderer::preCommit();

    auto &renderer = handle();
    if(hasChild("map_backplate")) {
      auto &cppTex = child("map_backplate").valueAs<cpp::Texture>();
      renderer.setParam("map_backplate", cppTex);
    }
  }

  }  // namespace sg
} // namespace ospray
