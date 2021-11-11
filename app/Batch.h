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
#include "sg/scene/Animation.h"
#include "sg/importer/Importer.h"

using namespace rkcommon::math;
using namespace ospray::sg;
using rkcommon::make_unique;

class BatchContext : public StudioContext
{
 public:
  BatchContext(StudioCommon &studioCommon);
  ~BatchContext() {}

  void start() override;
  void addToCommandLine(std::shared_ptr<CLI::App> app) override;
  bool parseCommandLine() override;
  void importFiles(sg::NodePtr world) override;
  void refreshRenderer();
  void refreshScene(bool resetCam) override;
  void updateCamera() override;
  void setCameraState(CameraState &cs) override;
  void render();
  void renderFrame();
  void renderAnimation();
  bool refreshCamera(int cameraIdx, bool resetArcball = false);
  void reshape();

 protected:
  NodePtr importedModels;
  bool cmdlCam{false};
  vec3f pos, up{0.f, 1.f, 0.f}, gaze{0.f, 0.f, 1.f};
  bool saveAlbedo{false};
  bool saveDepth{false};
  bool saveNormal{false};
  bool saveLayers{false};
  bool saveMetaData{true};
  std::string optImageFormat{"png"};

  int fps{0};
  bool forceRewrite{false};
  range1i framesRange{0, 0};
  range1i cameraRange{0, 0};
  bool useCameraRange{false};

  // list of cameras imported with the scene definition
  std::vector<sg::NodePtr> cameras;
  std::string cameraId{""};

  std::vector<CameraState> cameraStack;

  //camera animation
  sg::NodePtr selectedSceneCamera;

  float lockAspectRatio = 0.0;
  bool useArcball{false};
};
