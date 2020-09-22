// Copyright 2018-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "ospStudio.h"

using namespace ospray;
using rkcommon::removeArgs;

static std::vector<std::string> pluginsToLoad;

int main(int argc, const char *argv[])
{
  std::cout << "OSPRay Studio\n";

  // Parse first argument as StudioMode
  // (GUI is the default if no mode is given)
  // XXX Switch to using ospcommon/rkcommon ArgumentList
  auto mode = StudioMode::GUI;
  if (argc > 1) {
    auto modeArg = std::string(argv[1]);
    if (modeArg.front() != '-') {
      auto s = StudioModeMap.find(modeArg);
      if (s != StudioModeMap.end()) {
        mode = s->second;
        // Remove mode argument
        removeArgs(argc, argv, 1, 1);
      }
    }

    // Parse argument list for any plugins.
    // These need to be loaded before initializeOSPRay, as they can affect
    // import types.
    for (int i = 1; i < argc; i++) {
      const auto arg = std::string(argv[i]);
      if (arg == "--plugin" || arg == "-p") {
        pluginsToLoad.emplace_back(argv[i + 1]);
        removeArgs(argc, argv, i, 2);
        --i;
      }
    }
  }

  // load plugins //
  PluginManager pluginManager;
  for (auto &p : pluginsToLoad)
    pluginManager.loadPlugin(p);

  // Initialize OSPRay
  initializeOSPRay(argc, argv);

  // Check for module denoiser support after iniaitlizing OSPRay
  bool denoiser = ospLoadModule("denoiser") == OSP_NO_ERROR;
  std::cout << "OpenImageDenoise is" << (denoiser ? " " : "not ") << "available"
            << std::endl;

  // Set paramters common to all modes
  // doing so after initializeOSPRay allows OSPRay to remove its cmdline params.
  StudioCommon studioCommon(pluginManager, denoiser, argc, argv);

  // XXX Modes should be module loaded, statically linked causes
  // non-gui modes to still require glfw/GL
  switch (mode) {
  case StudioMode::GUI:
    start_GUI_mode(studioCommon);
    break;
  case StudioMode::BATCH:
    start_Batch_mode(studioCommon);
    break;
  case StudioMode::HEADLESS:
    std::cerr << "Headless mode\n";
    break;
  case StudioMode::TIMESERIES:
    start_TimeSeries_mode(studioCommon);
    break;
  default:
    std::cerr << "unknown mode!  How did I get here?!\n";
  }

  ospShutdown();

  return 0;
}
