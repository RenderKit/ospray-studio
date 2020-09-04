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
// ospcommon
#include "rkcommon/common.h"

#include "PluginManager.h"

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

  StudioCommon(PluginManager &_pluginManager, const bool denoiser,
               int _argc, const char **_argv) :
    pluginManager(_pluginManager), denoiserAvailable(denoiser),
    argc(_argc), argv(_argv) {};

  PluginManager &pluginManager;
  bool denoiserAvailable{false};
  const vec2i defaultSize = vec2i(1024, 768);

  int argc;
  const char **argv;
};

// Mode entry points
void start_GUI_mode(StudioCommon &studioCommon);
void start_Batch_mode(StudioCommon &studioCommon);
void start_TimeSeries_mode(StudioCommon &studioCommon);

inline void initializeOSPRay(
    int &argc, const char **argv, bool errorsFatal = true)
{
  // initialize OSPRay; OSPRay parses (and removes) its commandline parameters,
  // e.g. "--osp:debug"
  OSPError initError = ospInit(&argc, argv);

  if (initError != OSP_NO_ERROR)
    throw std::runtime_error("OSPRay not initialized correctly!");

  OSPDevice device = ospGetCurrentDevice();
  if (!device)
    throw std::runtime_error("OSPRay device could not be fetched!");

  // set an error callback to catch any OSPRay errors and exit the application
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
}
