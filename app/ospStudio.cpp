// Copyright 2018-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "ospStudio.h"

#include "MainWindow.h"
#include "Batch.h"
#include "TimeSeriesWindow.h"

using namespace ospray;
using rkcommon::removeArgs;

static std::vector<std::string> pluginsToLoad;

int main(int argc, const char *argv[])
{
  std::cout << "OSPRay Studio" << std::endl;

  // Just look for either version or verify_install arguments, initializeOSPRay
  // will remove OSPRay specific args, parse fully down further
  bool version = false;
  bool verify_install = false;
  for (int i = 1; i < argc; i++) {
    const auto arg = std::string(argv[i]);
    if (arg == "--version") {
      removeArgs(argc, argv, i, 1);
      version = true;
    }
    else if (arg == "--verify_install") {
      verify_install = true;
      removeArgs(argc, argv, i, 1);
    }
  }

  if (version) {
    std::cout << " OSPRay Studio version: " << OSPRAY_STUDIO_VERSION
              << std::endl;
    return 0;
  }

  // Initialize OSPRay
  OSPError error = initializeOSPRay(argc, argv);

  // Verify install then exit
  if (verify_install) {
    ospShutdown();
    auto fail = error != OSP_NO_ERROR;
    std::cout << " OSPRay Studio install " << (fail ? "failed" : "verified")
              << std::endl;
    return fail;
  }

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

    // Check for module denoiser support after iniaitlizing OSPRay
    bool denoiser = ospLoadModule("denoiser") == OSP_NO_ERROR;
    std::cout << "OpenImageDenoise is " << (denoiser ? "" : "not ")
              << "available" << std::endl;

  // This scope contains all OSPRay API calls. It enforces cleanly calling all
  // destructors before calling ospShutdown()
  {
    // Set paramaters common to all modes
    // doing so after initializeOSPRay allows OSPRay to remove its cmdline
    // params.
    StudioCommon studioCommon(pluginManager, denoiser, argc, argv);
    std::shared_ptr<StudioContext> context = nullptr;

    // XXX Modes should be module loaded, statically linked causes
    // non-gui modes to still require glfw/GL
    switch (mode) {
    case StudioMode::GUI:
      context = std::make_shared<MainWindow>(studioCommon);
      break;
    case StudioMode::BATCH:
      context = std::make_shared<BatchContext>(studioCommon);
      break;
    case StudioMode::HEADLESS:
      std::cerr << "Headless mode\n";
      break;
    case StudioMode::TIMESERIES:
      context = std::make_shared<TimeSeriesWindow>(studioCommon);
      break;
    default:
      std::cerr << "unknown mode!  How did I get here?!\n";
    }

    if (context)
      context->start();
  }

  ospShutdown();

  return 0;
}
