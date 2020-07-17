// ======================================================================== //
// Copyright 2018 Intel Corporation                                         //
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
// imgui
#include "imgui.h"
// rkcommon
#include "rkcommon/tasking/parallel_for.h"

using namespace ospray::sg;

bool denoiser = ospLoadModule("denoiser") == OSP_NO_ERROR;

// TimeSeriesWindow mode entry point
void start_TimeSeries_mode(int argc, const char *argv[])
{
  std::cout << "***** Timeseries Mode *****" << std::endl;
  MainWindow *mw = new MainWindow(vec2i(1024, 768), denoiser);
  auto window    = rkcommon::make_unique<TimeSeriesWindow>(mw);
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

TimeSeriesWindow::TimeSeriesWindow(MainWindow *mainWindow)
    : MainWindow(mainWindow)
{
  activeMainWindow = mainWindow;
  PathtracerParameters ptParams;
  g_pathtracerParameters = ptParams;
}

TimeSeriesWindow::~TimeSeriesWindow() {}

void TimeSeriesWindow::mainLoop()
{
  // generate timeseries data  and create necessary SG objects here.

  if (allVariablesData.size() == 0) {
    throw std::runtime_error("no data provided!");
  }

  int numTimesteps = allVariablesData[0].size();

  for (int i = 0; i < allVariablesData.size(); i++) {
    if (numTimesteps != allVariablesData[i].size()) {
      throw std::runtime_error("inconsistent number of timesteps per variable");
    }
  }

  std::cerr << "loaded " << numTimesteps << " timesteps across "
            << allVariablesData.size() << " variables" << std::endl;

  // one global frame
  frame = activeMainWindow->getFrame();

  // set renderer 
  frame->createChild("renderer", "renderer_" + rendererTypeStr);

  // generate one world per timestep
  for (int i = 0; i < allVariablesData[0].size(); i++) {
    auto world = std::static_pointer_cast<ospray::sg::World>(
        createNode("world", "world"));
    g_allWorlds.push_back(world);
    if (rendererTypeStr == "pathtracer") {
      world->createChild("light", "distant");
    }
  }

  // pre generate volumes/data for every timestep/world
  for (int i = 0; i < allVariablesData.size(); i++) {
    for (int f = 0; f < allVariablesData[i].size(); f++) {
      auto volumeTimestep = VolumeTimestep(allVariablesData[i][f],
                                           voxelType,
                                           dimensions,
                                           gridOrigin,
                                           gridSpacing);

      auto vol = volumeTimestep.createSGVolume();

      auto tfn = std::static_pointer_cast<sg::TransferFunction>(
          sg::createNode("tfn_" + to_string(i), "transfer_function_jet"));

      tfn->add(vol);

      for (int i = 0; i < numInstances; i++) {
        auto xfm  = affine3f::translate(vec3f(i, 0, 0)) * affine3f{one};
        auto newX = createNode("geomXfm" + to_string(i), "Transform", xfm);
        newX->add(vol);
        tfn->add(newX);
      }

      auto &world = g_allWorlds[f];

      world->add(tfn);
    }
  }

  // set initial timestep
  setTimestep(0);

  activeMainWindow->registerImGuiCallback([&]() {
    addTimeseriesUI();

    bool volumeParametersChanged = addVolumeUI(false);

    bool pathtracerParametersChanged = addPathTracerUI(false);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    static int spp = 1;

    if (ImGui::SliderInt("spp", &spp, 1, 32)) {
      renderer->createChild("spp", "int", spp);
    }

    if (volumeParametersChanged || pathtracerParametersChanged) {
      this->resetAccumulation();
    }
  });

  activeMainWindow->registerDisplayCallback(
      [&](MainWindow *) { animateTimesteps(); });

  // activeMainWindow->registerKeyCallback(
  //     std::function<void(
  //         MainWindow *, int key, int scancode, int action, int
  //         mods)> keyCallback);

  activeMainWindow->timeseriesMode = true;
  activeMainWindow->mainLoop();
}

void TimeSeriesWindow::updateWindowTitle(std::string &updatedTitle)
{
  int numTimesteps = g_allVariablesTimeseries[0].size();
  std::stringstream windowTitle;
  windowTitle << "Stellar radiation timestep "
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

    std::cerr << "usage: " << argv[0] << " -renderer scivis | pathtracer"
              <<" -dimensions <dimX> <dimY> <dimZ> "
              << "[-voxelType OSP_FLOAT | OSP_INT] "
              << "[-gridOrigin <x> <y> <z>] "
                 "[-gridSpacing <x> <y> <z>] <-variable <float.raw>>... "
              << "[-localLoading]" << std::endl;
    return 1;
  }
  while (argIndex < argc) {
    std::string switchArg(argv[argIndex++]);

    if (switchArg == "-renderer") {
      rendererTypeStr = argv[argIndex++];
    }

    else if (switchArg == "-numInstances") {
      numInstances = stoi(std::string(argv[argIndex++]));
    }
    
    else if (switchArg == "-dimensions") {
      if (argc < argIndex + 3) {
        throw std::runtime_error("improper -dimensions arguments");
      }

      const std::string dimX(argv[argIndex++]);
      const std::string dimY(argv[argIndex++]);
      const std::string dimZ(argv[argIndex++]);

      dimensions = vec3i(stoi(dimX), stoi(dimY), stoi(dimZ));
    }

    else if (switchArg == "-voxelType") {
      if (argc < argIndex + 1) {
        throw std::runtime_error("improper -voxelType arguments");
      }
      auto voxelTypeStr = std::string(argv[argIndex++]);
      auto it           = tableDataType.find(voxelTypeStr);
      if (it != tableDataType.end()) {
        voxelType = it->second;
      } else {
        throw std::runtime_error("improper -voxelType format requested");
      }
    }

    else if (switchArg == "-gridSpacing") {
      if (argc < argIndex + 3) {
        throw std::runtime_error("improper -gridSpacing arguments");
      }

      const std::string gridSpacingX(argv[argIndex++]);
      const std::string gridSpacingY(argv[argIndex++]);
      const std::string gridSpacingZ(argv[argIndex++]);

      gridSpacing =
          vec3f(stof(gridSpacingX), stof(gridSpacingY), stof(gridSpacingZ));
    }

    else if (switchArg == "-gridOrigin") {
      if (argc < argIndex + 3) {
        throw std::runtime_error("improper -gridOrigin arguments");
      }

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

      if (dimensions.x == -1) {
        throw std::runtime_error("improper dimensions specified for volume");
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
    if (activeMainWindow) {
      // activeMainWindow()->setPauseRendering(
      //     g_timeseriesParameters.pauseRendering);
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

bool TimeSeriesWindow::addVolumeUI(bool changed)
{
  static bool initialized = false;

  for (int i = 0; i < g_volumeParameters.size(); i++) {
    std::stringstream ss;
    ss << "[" << i << "]";

    if (ImGui::SliderFloat((std::string("sigmaTScale") + ss.str()).c_str(),
                           &g_volumeParameters[i].sigmaTScale,
                           0.001f,
                           100.f)) {
      changed = true;
    }

    if (ImGui::SliderFloat((std::string("sigmaSScale") + ss.str()).c_str(),
                           &g_volumeParameters[i].sigmaSScale,
                           0.01f,
                           1.f)) {
      changed = true;
    }

    if (ImGui::Checkbox((std::string("log scale") + ss.str()).c_str(),
                        &g_volumeParameters[i].useLogScale)) {
      changed = true;
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();
  }

  if (!initialized || changed) {
    std::vector<float> sigmaTScales, sigmaSScales;
    std::vector<int> useLogScales;

    for (int i = 0; i < g_volumeParameters.size(); i++) {
      sigmaTScales.push_back(g_volumeParameters[i].sigmaTScale);
      sigmaSScales.push_back(g_volumeParameters[i].sigmaSScale);
      useLogScales.push_back(g_volumeParameters[i].useLogScale);
    }

    // OSPData sigmaTScalesData =
    //     ospNewData(sigmaTScales.size(), OSP_FLOAT, sigmaTScales.data());
    // ospSetData(renderer, "sigmaTScales", sigmaTScalesData);
    // ospRelease(sigmaTScalesData);

    // OSPData sigmaSScalesData =
    //     ospNewData(sigmaSScales.size(), OSP_FLOAT, sigmaSScales.data());
    // ospSetData(renderer, "sigmaSScales", sigmaSScalesData);
    // ospRelease(sigmaSScalesData);

    // OSPData useLogScalesData =
    //     ospNewData(useLogScales.size(), OSP_INT, useLogScales.data());
    // ospSetData(renderer, "useLogScales", useLogScalesData);
    // ospRelease(useLogScalesData);

    // ospCommit(renderer);

    initialized = true;
  }

  return changed;
}

bool TimeSeriesWindow::addPathTracerUI(bool changed)
{
  if (rendererTypeStr == "pathtracer") {
    if (ImGui::SliderInt(
            "maxNumScatters", &g_pathtracerParameters.maxNumScatters, 1, 32)) {
      changed = true;
    }

    if (ImGui::SliderFloat("henyeyGreenCoeff",
                           &g_pathtracerParameters.henyeyGreenCoeff,
                           -1.f,
                           1.f)) {
      changed = true;
    }

    if (ImGui::SliderFloat("ambientLightIntensity",
                           &g_pathtracerParameters.ambientLightIntensity,
                           0.f,
                           10.f)) {
      changed = true;
    }

    if (ImGui::SliderFloat("directionalLightIntensity",
                           &g_pathtracerParameters.directionalLightIntensity,
                           0.f,
                           10.f)) {
      changed = true;
    }

    if (ImGui::SliderFloat(
            "directionalLightAngularDiameter",
            &g_pathtracerParameters.directionalLightAngularDiameter,
            0.f,
            180.f)) {
      changed = true;
    }

    if (ImGui::SliderFloat("directionalLightAzimuth",
                           &g_pathtracerParameters.directionalLightAzimuth,
                           -180.f,
                           180.f)) {
      changed = true;
    }

    if (ImGui::SliderFloat("directionalLightElevation",
                           &g_pathtracerParameters.directionalLightElevation,
                           -90.f,
                           90.f)) {
      changed = true;
    }
    if (changed) {
      renderer->createChild(
          "maxNumScatters", "int", g_pathtracerParameters.maxNumScatters);
      renderer->createChild(
          "henyeyGreenCoeff", "float", g_pathtracerParameters.henyeyGreenCoeff);
      renderer->createChild("ambientLightIntensity",
                            "float",
                            g_pathtracerParameters.ambientLightIntensity);
      renderer->createChild("directionalLightIntensity",
                            "float",
                            g_pathtracerParameters.directionalLightIntensity);
      renderer->createChild(
          "directionalLightAngularDiameter",
          "float",
          g_pathtracerParameters.directionalLightAngularDiameter);
      renderer->createChild("directionalLightAzimuth",
                            "float",
                            g_pathtracerParameters.directionalLightAzimuth);
      renderer->createChild("directionalLightElevation",
                            "float",
                            g_pathtracerParameters.directionalLightElevation);
    }
  }
  return changed;
}

void TimeSeriesWindow::setTimestepFb(int timestep)
{
  if (currentTimestep == timestep) {
    return;
  }

  currentTimestep = timestep;

  // SG frameBuffer operations here
  // use frame from mainwindow to do this

  if (framebuffersPerTimestep.count(currentTimestep) == 0) {
    std::shared_ptr<sg::FrameBuffer> currentFb =
        std::static_pointer_cast<sg::FrameBuffer>(
            sg::createNode("framebuffer", "framebuffer"));
    currentFb->createChild("colorFormat", "string", std::string("sRGB"));
    currentFb->createChild("size", "vec2i", framebufferSize);

    framebuffersPerTimestep[currentTimestep] = currentFb;

    framebufferLastReset[currentTimestep] = rkcommon::utility::TimeStamp();
  }

  framebuffer = framebuffersPerTimestep[currentTimestep];
  frame->add(framebuffer);

  if (framebufferLastReset.count(currentTimestep) == 0 ||
      framebufferLastReset[currentTimestep] < framebufferResetRequired) {
    framebuffer->resetAccumulation();
    framebufferLastReset[currentTimestep] = rkcommon::utility::TimeStamp();
  }
}

void TimeSeriesWindow::resetAccumulation()
{
  framebufferResetRequired = rkcommon::utility::TimeStamp();

  // reset accumulation for current frame buffer only
  if (framebuffer) {
    framebuffer->resetAccumulation();
    framebufferLastReset[currentTimestep] = framebufferResetRequired;
  }
}

bool TimeSeriesWindow::isTimestepLoaded(int timestep)
{
  bool loaded = true;

    if (timestep >= g_allWorlds.size()) {
      throw std::runtime_error("out of bounds timestep selected");


    if (!g_allWorlds[timestep]->hasChildren()) {
      loaded = false;
    }
  }

  return loaded;
}

void TimeSeriesWindow::setTimestep(int timestep)
{
  auto world = g_allWorlds[timestep];
  world->render();
  frame->add(world);

  frame->childAs<FrameBuffer>("framebuffer").resetAccumulation();
}

void TimeSeriesWindow::printHelp()
{
  std::cout <<
      R"text(
  ./ospStudio timeseries [parameters] [files]

  ospStudio timeseries specific parameters:
    -dimensions     <dimX> <dimY> <dimZ>
    -voxelType       OSP_FLOAT | 
    -gridSpacing    <x> <y> <z>
    -localLoading   true/false
    -variable       <float.raw>>... )text"
            << std::endl;
}
