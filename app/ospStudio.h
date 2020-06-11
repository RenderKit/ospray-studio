// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

// std
#include <iostream>
#include <map>
#include <stdexcept>
// ospray
#include "ospray/ospray_util.h"
// ospcommno
#include "ospcommon/common.h"

enum class StudioMode
{
  GUI,
  BATCH,
  HEADLESS
};

const static std::map<std::string, StudioMode> StudioModeMap = {
    {"gui", StudioMode::GUI},
    {"batch", StudioMode::BATCH},
    {"server", StudioMode::HEADLESS}};

// Mode entry points
void start_GUI_mode(int argc, const char *argv[]);
void start_Batch_mode(int argc, const char *argv[]);

inline void initializeOSPRay(int argc,
                             const char **argv,
                             bool errorsFatal = true)
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
  ospDeviceSetErrorFunc(device, [](OSPError error, const char *errorDetails) {
    std::cerr << "OSPRay error: " << errorDetails << std::endl;
    // exit(error); Not all errors need be fatal.  ie, ospLoadModule may fail.
  });

  ospDeviceSetStatusFunc(device, [](const char *message) {
    std::cout << "OSPRay message: " << message << std::endl;
  });
}