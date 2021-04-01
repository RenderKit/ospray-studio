// Copyright 2009-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ospStudio.h"

#include "ArcballCamera.h"
// ospray sg
#include "sg/Frame.h"
#include "sg/Node.h"
#include "sg/renderer/MaterialRegistry.h"
// Plugin
#include <chrono>
#include "PluginManager.h"
#include "sg/scene/Animation.h"
#include "sg/importer/Importer.h"

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
  void refreshRenderer();
  void refreshScene(bool resetCam) override;
  void updateCamera() override;
  void setCameraState(CameraState &cs) override;
  void render();
  void renderFrame();
  void renderAnimation();
  void refreshCamera(int cameraIdx);
  void applyCameraState(CameraState &cs);

 protected:
  PluginManager pluginManager;
  NodePtr importedModels;

  std::string optRendererTypeStr = "pathtracer";
  std::string optCameraTypeStr   = "perspective";
  std::string optImageName       = "ospBatch";
  vec2i optImageSize             = {1024, 768};
  int optSPP                     = 32;
  float optVariance              = 0.f; // varianceThreshold
  int optPF                      = -1; // use default
  int optDenoiser                = 0;
  bool optGridEnable             = false;
  vec3i optGridSize              = {1, 1, 1};
  // XXX should be OSPStereoMode, but for that we need 'uchar' Nodes
  int optStereoMode               = 0;
  float optInterpupillaryDistance = 0.0635f;
  bool cmdlCam{false};
  vec3f pos, up{0.f, 1.f, 0.f}, gaze{0.f, 0.f, 1.f};
  bool saveAlbedo{false};
  bool saveDepth{false};
  bool saveNormal{false};
  bool saveLayers{false};
  bool saveMetaData{false};
  std::string optImageFormat{"png"};
  bool animate{false};
  int fps{24};
  bool forceRewrite{false};
  range1i framesRange{0, 0};
  void printHelp() override;
  int cameraDef{0};
  range1i cameraRange{0, 0};
  bool useCameraRange{false};

  // list of cameras imported with the scene definition
  std::vector<sg::NodePtr> cameras;

  std::vector<CameraState> cameraStack;

  //camera animation
  sg::NodePtr selectedSceneCamera;
  bool animateCamera{false};

  //Volume parameters
  sg::NodePtr volumeParams;
};
