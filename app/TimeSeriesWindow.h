// ======================================================================== //
// Copyright 2020 Intel Corporation                                         //
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

#pragma once

#include <unordered_map>

#include "MainWindow.h"
#include "StateUtils.h"
#include "../sg/fb/FrameBuffer.h"
#include "../sg/Frame.h"
#include "../sg/scene/World.h"
#include "../sg/renderer/Renderer.h"
#include "../sg/visitors/PrintNodes.h"

using namespace std;

using namespace rkcommon::math;

class TimeSeriesWindow : public MainWindow
{
 public:
  TimeSeriesWindow();

  ~TimeSeriesWindow();

  bool parseCommandLine(int &ac, const char **&av);

  void mainLoop();

  void addTimeseriesUI();

  void animateTimesteps();

  bool addPathTracerUI(bool changed = false);

  bool addLightsUI(bool changed = false);

  void updateWindowTitle(std::string &updatedTitle);

  void setTimestepFb(int timestep);

  void resetAccumulation();

  bool isTimestepLoaded(int timestep);
  void setTimestep(int timestep);

  void printHelp();

  std::vector<ospray::sg::VolumeParameters> g_volumeParameters;
  std::vector<std::shared_ptr<ospray::sg::World>> g_allWorlds;

  std::vector<std::vector<std::string>> allVariablesData;

  // for each variable, time series of volume time steps
  int argIndex = 1;
  int voxelType;
  vec3i dimensions{-1};
  vec3f gridOrigin{0.f};
  vec3f gridSpacing{-1.f};

  int numInstances{1};

  std::string lightTypeStr{"sunSky"};
  std::string rendererTypeStr{"pathtracer"};

  bool setSeparateFramebuffers{false};

  struct TimeseriesParameters
  {
    int currentTimestep           = 0;
    bool playTimesteps            = false;
    int desiredTimestepsPerSecond = 1;

    int animationCenter       = 0;
    int animationNumTimesteps = 0;
    int animationIncrement    = 1;

    int computedAnimationMin = 0;
    int computedAnimationMax = 0;

    bool pauseRendering = false;
  };

  TimeseriesParameters g_timeseriesParameters;

 protected:
  float framebufferScale = 1.f;
  vec2i framebufferSize;

  int currentTimestep = 0;
  std::unordered_map<int, std::shared_ptr<sg::FrameBuffer>> framebuffersPerTimestep;

  // last time at which reset accumulation was required
  rkcommon::utility::TimeStamp framebufferResetRequired;

  // indicates when frame buffer for given time step was last reset
  std::unordered_map<int, rkcommon::utility::TimeStamp> framebufferLastReset;

  // allow window to redraw but do not render new frames (display current
  // framebuffer only)
  bool pauseRendering = false;

  // optional registered key callback, called when keys are pressed
  std::function<void(
      TimeSeriesWindow *, int key, int scancode, int action, int mods)>
      keyCallback;

  sg::PathtracerParameters g_pathtracerParameters;

  sg::LightParameters g_LightParameters;

  bool g_localLoading;
  bool g_lowResMode;
};
