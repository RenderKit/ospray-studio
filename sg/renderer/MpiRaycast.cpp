// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Renderer.h"

namespace ospray {
namespace sg {

struct OSPSG_INTERFACE MpiRaycast : public Renderer
{
  MpiRaycast();
  virtual ~MpiRaycast() override = default;
};

OSP_REGISTER_SG_NODE_NAME(MpiRaycast, renderer_mpiRaycast);

// MpiRaycast definitions ///////////////////////////////////////////////////////

MpiRaycast::MpiRaycast() : Renderer("mpiRaycast")
{
  createChild("aoSamples",
      "int",
      "number of rays per sample to compute ambient occlusion",
      0);
  createChild("aoDistance",
      "float",
      "maximum distance to consider for ambient occlusion",
      1e20f);
  createChild("volumeSamplingRate", "float", "sampling rate for volumes", 1.f);
  createChild("shadows", "bool", "whether to compute (hard) shadows", false);
  createChild("visibleLights",
      "bool",
      "whether light sources are potentially visible",
      false);

  child("aoSamples").setMinMax(0, 1000);
  child("aoDistance").setMinMax(0.f, 1e20f);
  child("volumeSamplingRate").setMinMax(0.f, 1e20f);
}

} // namespace sg
} // namespace ospray
