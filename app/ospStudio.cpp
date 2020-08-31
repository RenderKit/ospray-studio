// Copyright 2018-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "ospStudio.h"

using rkcommon::removeArgs;

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
        rkcommon::removeArgs(argc, argv, 1, 1);
      }
    }
  }

  initializeOSPRay(argc, argv);

  switch (mode) {
  case StudioMode::GUI:
    start_GUI_mode(argc, argv);
    break;
  case StudioMode::BATCH:
    start_Batch_mode(argc, argv);
    break;
  case StudioMode::HEADLESS:
    std::cerr << "Headless mode\n";
    break;
  case StudioMode::TIMESERIES:
    start_TimeSeries_mode(argc, argv);
    break;
  default:
    std::cerr << "unknown mode!  How did I get here?!\n";
  }

  ospShutdown();

  return 0;
}
