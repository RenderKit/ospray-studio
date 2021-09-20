// Copyright 2018-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "ospStudio.h"

#include "MainWindow.h"
#include "Batch.h"
#include "TimeSeriesWindow.h"
// CLI
#include <CLI11.hpp>

using namespace ospray;
using rkcommon::removeArgs;

void StudioContext::addToCommandLine(std::shared_ptr<CLI::App> app) {
  volumeParams = std::make_shared<sg::VolumeParams>();

  app->add_option(
    "files",
    filesToImport,
    "The list of files to import"
  );
  app->add_option(
    "--renderer",
    optRendererTypeStr,
    "set the renderer type"
  )->check(CLI::IsMember({"scivis", "pathtracer", "ao", "debug"}));
  app->add_option(
    "--pixelfilter",
    optPF,
    "set default pixel filter (0=point, 1=box, 2=Gaussian, 3=Mitchell-Netravali, 4=Blackman-Harris)"
  );
  app->add_option(
    "--resolution",
    [&](const std::vector<std::string> val) {
      std::string s = val[0];
      // s is one of: X"p", X"k", X"x"Y, X
      // e.g. 720p, 4K, 1024x768, 1024

      auto it = standardResolutionSizeMap.find(s);
      int found = s.find('x');

      if (it != standardResolutionSizeMap.end()) {
        // standard size: 720p, 1080p, etc
        optResolution = it->second;
      } else if (found != std::string::npos) {
        // exact resolution: 1024x768, 512x512, etc
        std::string width = s.substr(0, found);
        std::string height = s.substr(found + 1);
        optResolution = vec2i(std::stoi(width), std::stoi(height));
      } else {
        // exact square resolution: 1024, 512, etc
        optResolution = vec2i(std::stoi(s));
      }
      return true;
    },
    "Set the dimensions for imported volumes"
  );
  app->add_option(
    "--dimensions",
    [&](const std::vector<std::string> val) {
      auto dimensions = vec3i(std::stoi(val[0]), std::stoi(val[1]), std::stoi(val[2]));
      volumeParams->createChild("dimensions", "vec3i", dimensions);
      return true;
    },
    "Set the dimensions for imported volumes"
  )->expected(3);
  app->add_option(
    "--gridSpacing",
    [&](const std::vector<std::string> val) {
      auto gridSpacing = vec3f(std::stof(val[0]), std::stof(val[1]), std::stof(val[2]));
      volumeParams->createChild("gridSpacing", "vec3f", gridSpacing);
      return true;
    },
    "Set the grid spacing for imported volumes"
  )->expected(3);
  app->add_option(
    "--gridOrigin",
    [&](const std::vector<std::string> val) {
      auto gridOrigin = vec3f(std::stof(val[0]), std::stof(val[1]), std::stof(val[2]));
      volumeParams->createChild("gridSpacing", "vec3f", gridOrigin);
      return true;
    },
    "Set the grid origin for imported volumes"
  )->expected(3);
  app->add_option_function<OSPDataType>(
    "--voxelType",
    [&](const OSPDataType &voxelType) {
      volumeParams->createChild("voxelType", "int", (int)voxelType);
    },
    "Set the voxel type for imported volumes"
  )->transform(CLI::CheckedTransformer(sg::volumeVoxelType));
  app->add_option(
    "--sceneConfig",
    optSceneConfig,
    "Set the scene configuration (valid values: dynamic, compact, robust)"
  )->check(CLI::IsMember({"dynamic", "compact", "robust"}));
  app->add_option(
    "--instanceConfig",
    optInstanceConfig,
    "Set the instance configuration (valid values: dynamic, compact, robust)"
  )->check(CLI::IsMember({"dynamic", "compact", "robust"}));
  app->add_option(
    "--pointSize",
    pointSize,
    "Set the importer's point size"
  );
}

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

  // Check for module denoiser support after iniaitlizing OSPRay
  bool denoiser = ospLoadModule("denoiser") == OSP_NO_ERROR;
  std::cout << "OpenImageDenoise is " << (denoiser ? "" : "not ") << "available"
            << std::endl;

  // Parse first argument as StudioMode
  // (GUI is the default if no mode is given)
  // XXX Switch to using ospcommon/rkcommon ArgumentList
  auto mode = StudioMode::GUI;
  std::vector<std::string> pluginsToLoad;
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
    for (int i = 1; i < argc; i++) {
      const auto arg = std::string(argv[i]);
      if (arg == "--plugin" || arg == "-p") {
        pluginsToLoad.emplace_back(argv[i + 1]);
        removeArgs(argc, argv, i, 2);
        --i;
      }
    }
  }

  // Set parameters common to all modes
  StudioCommon studioCommon(pluginsToLoad, denoiser, argc, argv);

  // This scope contains all OSPRay API calls. It enforces cleanly calling all
  // destructors before calling ospShutdown()
  {
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
    else
      std::cerr << "Could not create a valid context. Stopping." << std::endl;
  }

  ospShutdown();

  return 0;
}
