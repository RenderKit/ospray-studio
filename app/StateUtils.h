// Copyright 2018 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once
#include "imgui.h"
#include "widgets/TransferFunctionWidget.h"

using namespace rkcommon::math;

namespace ospray {
  namespace sg {
  
struct VolumeParameters
{
  float sigmaTScale = 1.f;
  float sigmaSScale = 1.f;
  bool useLogScale  = true;
};

struct LightParameters
{
  float ambientLightIntensity           = 1.f;
  float directionalLightIntensity       = 1.f;
  float directionalLightAngularDiameter = 45.f;
  vec3f directionalLightDirection       = vec3f(0.f, 0.f, 1.f);
  float sunSkyAlbedo                    = 0.18f;
  float sunSkyTurbidity = 3.f;
  float sunSkyIntensity = 1.f;
  vec3f sunSkyColor = vec3f(1.f);
  vec3f sunSkyUp    = vec3f(0.f, 1.f, 0.f);
  vec3f sunSkyDirection = vec3f(0.f, 0.f, 1.f);
};

struct PathtracerParameters
{
  int lightSamples                    = 1;
  int roulettePathLength                = 5.f;
  float maxContribution                 = 0.f;
};

  struct State
  {
    State();
    ~State();

    std::vector<VolumeParameters> g_volumeParameters;
    PathtracerParameters g_pathtracerParameters;
    std::vector<std::string> g_stateFilenames;

    std::vector<std::shared_ptr<TransferFunctionWidget>>
        g_transferFunctionWidgets;
  };

}  // namespace sg
} // namespace ospray
