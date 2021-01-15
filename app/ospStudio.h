// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

// std
#include <iostream>
#include <map>
#include <stdexcept>
#include <vector>
// ospray
#include "ospray/ospray_util.h"
// ospray sg
#include "sg/Frame.h"
#include "sg/renderer/MaterialRegistry.h"
#include "sg/scene/lights/LightsManager.h"
// studio app
#include "ArcballCamera.h"
// ospcommon
#include "rkcommon/common.h"

#include "version.h"

using namespace ospray;
using namespace rkcommon::math;

enum class StudioMode
{
  GUI,
  BATCH,
  HEADLESS,
  TIMESERIES
};

const static std::map<std::string, StudioMode> StudioModeMap = {
    {"gui", StudioMode::GUI},
    {"batch", StudioMode::BATCH},
    {"server", StudioMode::HEADLESS},
    {"timeseries", StudioMode::TIMESERIES}};

// Common across all modes

class StudioCommon
{
 public:
  StudioCommon(std::vector<std::string> _pluginsToLoad,
      const bool denoiser,
      int _argc,
      const char **_argv)
      : pluginsToLoad(_pluginsToLoad),
        denoiserAvailable(denoiser),
        argc(_argc),
        argv(_argv){};

  std::vector<std::string> pluginsToLoad;
  bool denoiserAvailable{false};
  const vec2i defaultSize = vec2i(1024, 768);

  int argc;
  const char **argv;
};

// abstract base class for all Studio modes
// ALOK: should be merged with StudioCommon above
class StudioContext : public std::enable_shared_from_this<StudioContext>
{
 public:
  StudioContext(StudioCommon &_common)
    :studioCommon(_common)
  {
    frame = sg::createNodeAs<sg::Frame>("main_frame", "frame");
    baseMaterialRegistry = sg::createNodeAs<sg::MaterialRegistry>(
        "baseMaterialRegistry", "materialRegistry");
    lightsManager = sg::createNodeAs<sg::Lights>("lights", "lights");
  }

  virtual ~StudioContext() {}

  virtual void start() = 0;
  virtual bool parseCommandLine() = 0;
  virtual void importFiles(sg::NodePtr world) = 0;
  virtual void refreshScene(bool resetCam) = 0;
  virtual void updateCamera() = 0;

  // this method is so that importScene (in sg) does not need
  // to compile/link with ArcballCamera/UI
  virtual void setCameraState(CameraState &cs) = 0;

  std::shared_ptr<sg::Frame> frame;
  std::shared_ptr<sg::MaterialRegistry> baseMaterialRegistry;
  std::shared_ptr<sg::Lights> lightsManager;

  std::vector<std::string> filesToImport;
  std::unique_ptr<ArcballCamera> arcballCamera;

  int defaultMaterialIdx = 0;

 protected:
  virtual void printHelp()
  {
    std::cerr << "common Studio help message" << std::endl;
  }

  bool sgScene{false}; // whether we are loading a scene file

  StudioCommon &studioCommon;
};

inline OSPError initializeOSPRay(
    int &argc, const char **argv)
{
  // initialize OSPRay; OSPRay parses (and removes) its commandline parameters,
  // e.g. "--osp:debug"
  OSPError initError = ospInit(&argc, argv);

  if (initError != OSP_NO_ERROR) {
    std::cerr << "OSPRay not initialized correctly!" << std::endl;
    return initError;
  }

  OSPDevice device = ospGetCurrentDevice();
  if (!device) {
    std::cerr << "OSPRay device could not be fetched!" << std::endl;
    return OSP_UNKNOWN_ERROR;
  }

  // set an error callback to catch any OSPRay errors
  ospDeviceSetErrorCallback(
      device,
      [](void *, OSPError, const char *errorDetails) {
        std::cerr << "OSPRay error: " << errorDetails << std::endl;
      },
      nullptr);

  ospDeviceSetStatusCallback(
      device, [](void *, const char *msg) { std::cout << msg; }, nullptr);

  bool warnAsErrors = true;
  auto logLevel = OSP_LOG_WARNING;

  ospDeviceSetParam(device, "warnAsError", OSP_BOOL, &warnAsErrors);
  ospDeviceSetParam(device, "logLevel", OSP_INT, &logLevel);

  ospDeviceCommit(device);
  ospDeviceRelease(device);

  return OSP_NO_ERROR;
}
