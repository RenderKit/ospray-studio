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

#include "TimeSeriesWindow.h"
#include "../sg/scene/transfer_function/TransferFunction.h"
#include "sg/scene/lights/Lights.h"
// imgui
#include "imgui.h"
// rkcommon
#include "rkcommon/tasking/parallel_for.h"

#include "../sg/scene/volume/VDBVolumeTimeStep.h"
#include "../sg/scene/volume/VolumeTimeStep.h"

using namespace ospray::sg;

bool denoiser = ospLoadModule("denoiser") == OSP_NO_ERROR;
vec2i windowSize = vec2i(1024, 1024);

// TimeSeriesWindow mode entry point
void start_TimeSeries_mode(int argc, const char *argv[])
{
  std::cout << "***** Timeseries Mode *****" << std::endl;
  auto window    = rkcommon::make_unique<TimeSeriesWindow>();
  window->parseCommandLine(argc, argv);
  window->mainLoop();
  window.reset();
}

static std::unordered_map<std::string, OSPDataType> const tableDataType = {
    {"float", OSP_FLOAT},
    {"int", OSP_INT},
    {"uchar", OSP_UCHAR},
    {"short", OSP_SHORT},
    {"ushort", OSP_USHORT},
    {"double", OSP_DOUBLE}};

TimeSeriesWindow::TimeSeriesWindow() 
{}

TimeSeriesWindow::~TimeSeriesWindow() {}

bool TimeSeriesWindow::isTimestepVolumeLoaded(int variableNum, int timestep)
{
  bool loaded = true;

  for (int v = 0; v < allVariablesData[variableNum].size(); v++) {
    if (timestep >= allVariablesData[variableNum].size()) {
      throw std::runtime_error("out of bounds timestep selected");
    }

    if (!g_allWorlds[timestep]->hasChild("sgVolume_" + to_string(variableNum))) {
      loaded = false;
    }
  }

  return loaded;
}

void TimeSeriesWindow::mainLoop()
{
  // generate timeseries data  and create necessary SG objects here.

  if (allVariablesData.size() == 0) {
    throw std::runtime_error("no data provided!");
  }

  if (rendererTypeStr == "scivis" && lightTypeStr != "ambient") {
    throw std::runtime_error("wrong renderer and light type combination");
  }

  int numTimesteps = allVariablesData[0].size();

  for (int i = 0; i < allVariablesData.size(); i++) {
    if (numTimesteps != allVariablesData[i].size()) {
      throw std::runtime_error("inconsistent number of timesteps per variable");
    }
  }

  std::cerr << "loaded " << numTimesteps << " timesteps across "
            << allVariablesData.size() << " variables" << std::endl;

  auto frame = this->activeWindow->getFrame();
  frame->immediatelyWait = true;

  this->activeWindow->timeseriesMode = true;
  this->activeWindow->rendererTypeStr = rendererTypeStr;
  this->activeWindow->refreshRenderer();

  auto &lightMan = frame->createChildAs<sg::Lights>("lights", "lights");
  lightMan.addLight(lightTypeStr, lightTypeStr);
  this->activeWindow->lightTypeStr = lightTypeStr;
  lightMan.removeLight("ambient");

  // generate one world per timestep
  for (int i = 0; i < allVariablesData[0].size(); i++) {
    auto world = std::static_pointer_cast<ospray::sg::World>(
        createNode("world", "world"));
    g_allWorlds.push_back(world);
  }

  // pre generate volumes/data for every timestep/world
  for (int i = 0; i < allVariablesData.size(); i++) {
    for (int f = 0; f < allVariablesData[i].size(); f++) {

      std::shared_ptr<sg::Volume> vol;

      if (allVariablesData[i][f].length() > 4 &&
          allVariablesData[i][f].substr(allVariablesData[i][f].length() - 4) ==
              ".vdb") {

        auto vdbVolumeTimestep = VDBVolumeTimestep(allVariablesData[i][f]);
        vdbVolumeTimestep.localLoading = g_localLoading;
        vdbVolumeTimestep.variableNum  = i;

        if (!g_localLoading && !isTimestepVolumeLoaded(i, f)) {
          vdbVolumeTimestep.queueGenerateVolumeData();
          vdbVolumeTimestep.waitGenerateVolumeData();
        }

        vol                            = vdbVolumeTimestep.createSGVolume();

      } else {

        if (dimensions.x == -1 || gridSpacing.x == -1) {
        throw std::runtime_error("improper dimensions or grid spacing specified for volume");
        } 
      if (voxelType == 0)
        throw std::runtime_error("improper voxelType specified for volume");

        auto volumeTimestep = VolumeTimestep(allVariablesData[i][f],
                                             voxelType,
                                             dimensions,
                                             gridOrigin,
                                             gridSpacing);
        volumeTimestep.localLoading = g_localLoading;
        volumeTimestep.variableNum  = i;
       
        if (!g_localLoading && !isTimestepVolumeLoaded(i, f)) {
          volumeTimestep.queueGenerateVolumeData();
          volumeTimestep.waitGenerateVolumeData();
        }

        vol                         = volumeTimestep.createSGVolume();
      }

      auto tfn = std::static_pointer_cast<sg::TransferFunction>(
          sg::createNode("tfn_" + to_string(i), "transfer_function_cloud"));

      tfn->add(vol);

      for (int i = 0; i < numInstances; i++) {
        auto xfm  = affine3f::translate(vec3f(i + 2*i, 0, 0)) * affine3f{one};
        auto newX = createNode("geomXfm" + to_string(i), "Transform", xfm);
        newX->add(vol);
        tfn->add(newX);
      }

      auto &world = g_allWorlds[f];

      world->add(tfn);
      world->render();
    }
  }

  this->activeWindow->arcballCamera.reset(
      new ArcballCamera(g_allWorlds[0]->bounds(), windowSize));
  this->activeWindow->updateCamera();

  // set initial timestep
  setTimestep(0);

  this->activeWindow->registerImGuiCallback([&]() {
    addTimeseriesUI();
  });

  this->activeWindow->registerDisplayCallback(
      [&](MainWindow *) { animateTimesteps(); });

  // this->activeWindow->registerKeyCallback(
  //     std::function<void(
  //         MainWindow *, int key, int scancode, int action, int
  //         mods)> keyCallback);

  this->activeWindow->mainLoop();
}

void TimeSeriesWindow::updateWindowTitle(std::string &updatedTitle)
{
  int numTimesteps = g_allWorlds.size();
  std::stringstream windowTitle;
  windowTitle << "Cloud Demo timestep "
              << g_timeseriesParameters.currentTimestep << " / "
              << numTimesteps - 1;

  if (g_lowResMode) {
    windowTitle << "*";
  }
  updatedTitle = windowTitle.str();
}

bool TimeSeriesWindow::parseCommandLine(int &argc, const char **&argv)
{
  if (argc < 2) {
    std::cout << "incomplete/wrong usage of command line params" << std::endl;

    std::cerr << "usage: " << argv[0] << " timeseries"
              <<" [-renderer scivis | pathtracer ]"
              <<" [-dimensions <dimX> <dimY> <dimZ>] "
              << "[-voxelType OSP_FLOAT | OSP_INT] "
              << "[-gridOrigin <x> <y> <z>] "
                 "[-gridSpacing <x> <y> <z>] "
              << "[-numInstances <n>] "
              << "[-variable <float.raw>>...]" << std::endl;
    return 1;
  }
  while (argIndex < argc) {
    std::string switchArg(argv[argIndex++]);

    if (switchArg == "-renderer") {
      // XXX This doesn't play nicely with the whichRenderer int in MainWindow
      rendererTypeStr = argv[argIndex++];
    }

    else if (switchArg == "-light") {
      // XXX This doesn't play nicely with the whichLight int in MainWindow
      lightTypeStr = argv[argIndex++];
    }

    else if (switchArg == "-numInstances") {
      numInstances = stoi(std::string(argv[argIndex++]));
    }
    
    else if (switchArg == "-dimensions") {

      const std::string dimX(argv[argIndex++]);
      const std::string dimY(argv[argIndex++]);
      const std::string dimZ(argv[argIndex++]);

      dimensions = vec3i(stoi(dimX), stoi(dimY), stoi(dimZ));
    }

    else if (switchArg == "-voxelType") {
      auto voxelTypeStr = std::string(argv[argIndex++]);
      auto it           = tableDataType.find(voxelTypeStr);
      if (it != tableDataType.end()) {
        voxelType = it->second;
      } else {
        throw std::runtime_error("improper -voxelType format requested");
      }
    }

    else if (switchArg == "-gridSpacing") {

      const std::string gridSpacingX(argv[argIndex++]);
      const std::string gridSpacingY(argv[argIndex++]);
      const std::string gridSpacingZ(argv[argIndex++]);

      gridSpacing =
          vec3f(stof(gridSpacingX), stof(gridSpacingY), stof(gridSpacingZ));
    }

    else if (switchArg == "-gridOrigin") {

      const std::string gridOriginX(argv[argIndex++]);
      const std::string gridOriginY(argv[argIndex++]);
      const std::string gridOriginZ(argv[argIndex++]);

      gridOrigin =
          vec3f(stof(gridOriginX), stof(gridOriginY), stof(gridOriginZ));
    }

    else if (switchArg == "-variable") {
      if (argc < argIndex + 1) {
        throw std::runtime_error("improper -variable arguments");
      }

      // fit it into a unit cube (if no other grid spacing provided)
      if (gridSpacing == vec3f(-1.f)) {
        const float normalizedGridSpacing = reduce_min(1.f / dimensions);

        gridOrigin  = vec3f(-0.5f * dimensions * normalizedGridSpacing);
        gridSpacing = vec3f(normalizedGridSpacing);
      }

      std::vector<std::string> singleVariableData;


      while (argIndex < argc && argv[argIndex][0] != '-') {
        const std::string filename(argv[argIndex++]);
        singleVariableData.push_back(filename);
      }

      allVariablesData.push_back(singleVariableData);
    }

    else if (switchArg == "-localLoading") {
      g_localLoading = true;
    } else {
      // Ignore "--osp:" ospray arguments
      if (switchArg.rfind("--osp:") != std::string::npos)
        continue;
      std::cerr << "switch arg: " << switchArg << std::endl;
      throw std::runtime_error("unknown switch argument");
    }
  }

  std::cerr << "local loading for time steps (on each process): "
            << g_localLoading << std::endl;

  return 1;
}

void TimeSeriesWindow::addTimeseriesUI()
{
  auto frame = this->activeWindow->getFrame();
  ImGui::Begin("Time series");

  int numTimesteps = g_allWorlds.size();

  if (g_timeseriesParameters.playTimesteps) {
    if (ImGui::Button("Pause")) {
      g_timeseriesParameters.playTimesteps = false;
    }
  } else {
    if (ImGui::Button("Play")) {
      g_timeseriesParameters.playTimesteps = true;
    }
  }

  ImGui::SameLine();

  if (ImGui::SliderInt("Timestep",
                       &g_timeseriesParameters.currentTimestep,
                       0,
                       numTimesteps - 1)) {
    setTimestep(g_timeseriesParameters.currentTimestep);
  }

  ImGui::InputInt("Timesteps per second",
                  &g_timeseriesParameters.desiredTimestepsPerSecond);

  ImGui::SliderInt("Animation center",
                   &g_timeseriesParameters.animationCenter,
                   0,
                   numTimesteps - 1);

  // one time initialization
  if (g_timeseriesParameters.animationNumTimesteps == 0) {
    g_timeseriesParameters.animationNumTimesteps = numTimesteps;
  }

  ImGui::SliderInt("Animation num timesteps",
                   &g_timeseriesParameters.animationNumTimesteps,
                   1,
                   numTimesteps - 1);

  ImGui::SliderInt(
      "Animation increment", &g_timeseriesParameters.animationIncrement, 1, 16);

  // compute actual animation bounds
  g_timeseriesParameters.computedAnimationMin =
      g_timeseriesParameters.animationCenter -
      0.5 * g_timeseriesParameters.animationNumTimesteps *
          g_timeseriesParameters.animationIncrement;

  g_timeseriesParameters.computedAnimationMax =
      g_timeseriesParameters.animationCenter +
      0.5 * g_timeseriesParameters.animationNumTimesteps *
          g_timeseriesParameters.animationIncrement;

  if (g_timeseriesParameters.computedAnimationMin < 0) {
    int delta = -g_timeseriesParameters.computedAnimationMin;
    g_timeseriesParameters.computedAnimationMin = 0;
    g_timeseriesParameters.computedAnimationMax += delta;
  }

  if (g_timeseriesParameters.computedAnimationMax > numTimesteps - 1) {
    int delta =
        g_timeseriesParameters.computedAnimationMax - (numTimesteps - 1);
    g_timeseriesParameters.computedAnimationMax = numTimesteps - 1;
    g_timeseriesParameters.computedAnimationMin -= delta;
  }

  g_timeseriesParameters.computedAnimationMin =
      std::max(0, g_timeseriesParameters.computedAnimationMin);
  g_timeseriesParameters.computedAnimationMax =
      std::min(numTimesteps - 1, g_timeseriesParameters.computedAnimationMax);

  ImGui::Text("animation range: %d -> %d",
              g_timeseriesParameters.computedAnimationMin,
              g_timeseriesParameters.computedAnimationMax);

  ImGui::Spacing();

  if (ImGui::Checkbox("pause rendering",
                      &g_timeseriesParameters.pauseRendering)) {
    if (this->activeWindow) {
      frame->pauseRendering = 
          g_timeseriesParameters.pauseRendering;
    }
  }

  ImGui::End();
}

void TimeSeriesWindow::animateTimesteps()
{
  int numTimesteps = g_allWorlds.size();

  if (numTimesteps != 1 && g_timeseriesParameters.playTimesteps) {
    static auto timestepLastChanged = std::chrono::system_clock::now();
    double minChangeInterval =
        1. / double(g_timeseriesParameters.desiredTimestepsPerSecond);
    auto now = std::chrono::system_clock::now();

    std::chrono::duration<double> elapsedSeconds = now - timestepLastChanged;

    if (elapsedSeconds.count() > minChangeInterval) {
      g_timeseriesParameters.currentTimestep +=
          g_timeseriesParameters.animationIncrement;

      if (g_timeseriesParameters.currentTimestep <
          g_timeseriesParameters.computedAnimationMin) {
        g_timeseriesParameters.currentTimestep =
            g_timeseriesParameters.computedAnimationMin;
      }

      if (g_timeseriesParameters.currentTimestep >
          g_timeseriesParameters.computedAnimationMax) {
        g_timeseriesParameters.currentTimestep =
            g_timeseriesParameters.computedAnimationMin;
      }

      setTimestep(g_timeseriesParameters.currentTimestep);
      timestepLastChanged = now;
    }
  }
}

void TimeSeriesWindow::setTimestepFb(int timestep)
{
  auto frame = this->activeWindow->getFrame();
  if (currentTimestep == timestep) {
    return;
  }

  currentTimestep = timestep;

  if (framebuffersPerTimestep.count(currentTimestep) == 0) {
    std::shared_ptr<sg::FrameBuffer> currentFb =
        std::static_pointer_cast<sg::FrameBuffer>(
            sg::createNode("framebuffer", "framebuffer"));

    framebuffersPerTimestep[currentTimestep] = currentFb;

    framebufferLastReset[currentTimestep] = rkcommon::utility::TimeStamp();
  }

  frame->add(framebuffersPerTimestep[currentTimestep]);

  if (framebufferLastReset.count(currentTimestep) == 0 ||
      framebufferLastReset[currentTimestep] < framebufferResetRequired) {
    frame->childAs<FrameBuffer>("framebuffer").resetAccumulation();
    framebufferLastReset[currentTimestep] = rkcommon::utility::TimeStamp();
  }
}

void TimeSeriesWindow::resetAccumulation()
{
  auto frame               = this->activeWindow->getFrame();
  framebufferResetRequired = rkcommon::utility::TimeStamp();

  if (frame->hasChild("framebuffer")) {
    auto &framebuffer = frame->childAs<FrameBuffer>("framebuffer");

    // reset accumulation for current frame buffer only
    framebuffer.resetAccumulation();
    framebufferLastReset[currentTimestep] = framebufferResetRequired;
  }
}

void TimeSeriesWindow::setTimestep(int timestep)
{
  auto frame = this->activeWindow->getFrame();
  auto world = g_allWorlds[timestep];
  frame->add(world);

  frame->childAs<FrameBuffer>("framebuffer").resetAccumulation();

  //set up separate framebuffers
  if (setSeparateFramebuffers)
  setTimestepFb(timestep);
}

void TimeSeriesWindow::printHelp()
{
  std::cout <<
      R"text(
  ./ospStudio timeseries [parameters] [files]

  requirement for parameteres depend on the type of volume import.

  ospStudio timeseries specific parameters:
    -renderer       pathtracer | scivis
    -dimensions     <dimX> <dimY> <dimZ>
    -voxelType       OSP_FLOAT | 
    -gridSpacing    <x> <y> <z>
    -gridOrigin     <x> <y> <z>
    -numInstances   <number of instances>
    -variable       <float.extension>>... )text"
            << std::endl;
}
