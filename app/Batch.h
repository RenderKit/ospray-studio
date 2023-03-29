// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ospStudio.h"

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
  void render();
  virtual void renderFrame();
  void renderAnimation();
  void refreshCamera(int cameraIdx);
  void reshape();
  void loadCamJson();
 protected:
  NodePtr importedModels;
  bool cmdlCam{false};
  vec3f pos, up{0.f, 1.f, 0.f}, gaze{0.f, 0.f, 1.f};
  bool saveMetaData{true};

  float fps{0.0f};
  bool forceRewrite{false};
  range1i framesRange{0, -1}; // empty
  int frameStep{1};

  // list of cameras imported with the scene definition
  std::shared_ptr<CameraMap> cameras{nullptr};
  std::string cameraId{""};

  std::vector<affine3f> cameraStack;

  //camera animation
  sg::NodePtr selectedSceneCamera;

  float lockAspectRatio = 0.0;

  // SceneGraph
  bool saveScene{false};
};
