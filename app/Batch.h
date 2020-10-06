// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ospStudio.h"

#include "ArcballCamera.h"
// ospray sg
#include "sg/Frame.h"
#include "sg/Node.h"
#include "sg/renderer/MaterialRegistry.h"
// Plugin
#include "PluginManager.h"

using namespace rkcommon::math;
using namespace ospray;
using namespace ospray::sg;
using rkcommon::make_unique;

class BatchContext : public StudioContext
{
 public:
  BatchContext(StudioCommon &studioCommon);
  ~BatchContext() {}

  void start() override;
  bool parseCommandLine() override;
  void importFiles(sg::NodePtr world) override;
  void refreshScene(bool resetCam) override;
  void updateCamera() override;
  void setCameraState(CameraState &cs) override;
  void render();

 protected:
  PluginManager pluginManager;
  NodePtr importedModels;

  std::string optRendererTypeStr = "scivis";
  std::string optCameraTypeStr   = "perspective";
  std::string optImageName       = "ospBatch";
  vec2i optImageSize             = {1024, 768};
  int optSPP                     = 32;
  int optPF                      = -1; // use default
  int optDenoiser                = 0;
  bool optGridEnable             = false;
  vec3i optGridSize              = {1, 1, 1};
  // XXX should be OSPStereoMode, but for that we need 'uchar' Nodes
  int optStereoMode               = 0;
  float optInterpupillaryDistance = 0.0635f;
  bool cmdlCam{false};
  vec3f pos, up{0.f, 1.f, 0.f}, gaze{0.f, 0.f, 1.f};

  void printHelp() override;
};
