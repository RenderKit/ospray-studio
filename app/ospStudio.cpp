// ======================================================================== //
// Copyright 2018-2019 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

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
