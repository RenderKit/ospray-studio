// Copyright 2019-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "TimeSeriesWindow.h"
#include "sg/scene/transfer_function/TransferFunction.h"
#include "sg/scene/lights/LightsManager.h"
// imgui
#include "imgui.h"
// rkcommon
#include "rkcommon/tasking/parallel_for.h"
// json
#include "sg/JSONDefs.h"

#include <chrono>

#include "sg/scene/volume/VDBVolumeTimeStep.h"
#include "sg/scene/volume/VolumeTimeStep.h"
#include "sg/scene/volume/Volume.h"

// CLI
#include <CLI11.hpp>

using namespace ospray::sg;

std::vector<std::string> variablesLoaded;

TimeSeriesWindow::TimeSeriesWindow(StudioCommon &_common) 
  : MainWindow(_common)
{
  pluginManager = std::make_shared<PluginManager>();
}

TimeSeriesWindow::~TimeSeriesWindow() {}

void TimeSeriesWindow::start()
{
  std::cout << "Time Series mode" << std::endl;

  // load plugins //
  for (auto &p : studioCommon.pluginsToLoad)
    pluginManager->loadPlugin(p);

  // create panels //
  // doing this outside constructor to ensure shared_from_this()
  // can wrap a valid weak_ptr (in constructor, not guaranteed)

  pluginManager->main(shared_from_this(), &pluginPanels);
  // std::move(newPluginPanels.begin(),
  //           newPluginPanels.end(),
  //           std::back_inserter(pluginPanels));

  parseCommandLine();

  std::ifstream cams("cams.json");
  if (cams) {
    JSON j;
    cams >> j;
    cameraStack = j.get<std::vector<CameraState>>();
  }

  mainLoop();
}

bool TimeSeriesWindow::isTimestepVolumeLoaded(int variableNum, size_t timestep)
{
  bool loaded = true;

  for (size_t v = 0; v < allVariablesData[variableNum].size(); v++) {
    if (timestep >= allVariablesData[variableNum].size()) {
      throw std::runtime_error("out of bounds timestep selected");
    }
    if (importAsSeparateTimeseries)
      loaded = g_allSeparateWorlds[variableNum][timestep]->hasChild(
          "sgVolume_" + to_string(variableNum));

    else {
      if (!g_allWorlds[timestep]->hasChild("sgVolume_" + to_string(variableNum))) {
      loaded = false;
      }
    }
  }

  return loaded;
}

bool variableUI_callback(void *, int index, const char **out_text)
{
  *out_text = variablesLoaded[index].c_str();
  return true;
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

  size_t numTimesteps = allVariablesData[0].size();

  if (!importAsSeparateTimeseries)
    for (size_t i = 0; i < allVariablesData.size(); i++) {
      if (numTimesteps != allVariablesData[i].size()) {
        throw std::runtime_error(
            "inconsistent number of timesteps per variable");
      }
    }

  std::cerr << "loaded " << numTimesteps << " timesteps across "
            << allVariablesData.size() << " variables" << std::endl;

  auto frame = activeWindow->getFrame();
  frame->immediatelyWait = true;

  activeWindow->timeseriesMode = true;
  activeWindow->rendererTypeStr = rendererTypeStr;
  activeWindow->refreshRenderer();

  lightsManager->addLight(lightTypeStr, lightTypeStr);
  activeWindow->lightTypeStr = lightTypeStr;

  if (!importAsSeparateTimeseries) {
    // generate one world per timestep
    for (size_t i = 0; i < allVariablesData[0].size(); i++) {
      auto world = std::static_pointer_cast<ospray::sg::World>(
          createNode("world", "world"));
      g_allWorlds.push_back(world);
    }

    // pre generate volumes/data for every timestep/world
    for (size_t i = 0; i < allVariablesData.size(); i++) {
      rkcommon::FileName fileName(allVariablesData[i][0]);
      size_t lastindex = fileName.base().find_first_of(".");
      variablesLoaded.push_back(fileName.base().substr(0, lastindex));

      for (size_t f = 0; f < allVariablesData[i].size(); f++) {
        std::shared_ptr<sg::Volume> vol;

        if (allVariablesData[i][f].length() > 4
            && allVariablesData[i][f].substr(
                   allVariablesData[i][f].length() - 4)
                == ".vdb") {
          auto vdbVolumeTimestep = VDBVolumeTimestep(allVariablesData[i][f]);
          vdbVolumeTimestep.localLoading = g_localLoading;
          vdbVolumeTimestep.variableNum = i;

          if (!g_localLoading && !isTimestepVolumeLoaded(i, f)) {
            vdbVolumeTimestep.queueGenerateVolumeData();
            vdbVolumeTimestep.waitGenerateVolumeData();
          }

          vol = vdbVolumeTimestep.createSGVolume();
          vol->child("anisotropy").setValue(0.875f);
          vol->child("densityScale").setValue(1.f);

        } else {
          if (dimensions.x == -1 || gridSpacing.x == -1) {
            throw std::runtime_error(
                "improper dimensions or grid spacing specified for volume");
          }
          if (voxelType == 0)
            throw std::runtime_error("improper voxelType specified for volume");

          auto volumeTimestep = VolumeTimestep(allVariablesData[i][f],
              voxelType,
              dimensions,
              gridOrigin,
              gridSpacing);
          volumeTimestep.localLoading = g_localLoading;
          volumeTimestep.variableNum = i;

          if (!g_localLoading && !isTimestepVolumeLoaded(i, f)) {
            volumeTimestep.queueGenerateVolumeData();
            volumeTimestep.waitGenerateVolumeData();
          }

          vol = volumeTimestep.createSGVolume();
        }

        auto tfn = std::static_pointer_cast<sg::TransferFunction>(
            sg::createNode("tfn_" + to_string(i), "transfer_function_turbo"));

        for (int j = 0; j < numInstances; j++) {
          auto newX = createNode("geomXfm" + to_string(j), "transform");
          newX->child("translation") = vec3f(j + 20 * j + i * 10, 0, 0);
          newX->add(vol);
          tfn->add(newX);
        }

        auto &world = g_allWorlds[f];

        world->add(tfn);
        world->render();
      }
    }
  } else {
    // pre generate volumes/data for every timestep/world
    for (size_t i = 0; i < allVariablesData.size(); i++) {
      rkcommon::FileName fileName(allVariablesData[i][0]);
      size_t lastindex = fileName.base().find_first_of(".");
      variablesLoaded.push_back(fileName.base().substr(0, lastindex));

      // single variable world vector
      std::vector<std::shared_ptr<sg::World>> singleVariableWorlds;

      for (size_t f = 0; f < allVariablesData[i].size(); f++) {
        auto world = std::static_pointer_cast<ospray::sg::World>(
            createNode("world", "world"));
        singleVariableWorlds.push_back(world);

        std::shared_ptr<sg::Volume> vol;

        if (allVariablesData[i][f].length() > 4
            && allVariablesData[i][f].substr(
                   allVariablesData[i][f].length() - 4)
                == ".vdb") {
          auto vdbVolumeTimestep = VDBVolumeTimestep(allVariablesData[i][f]);
          vdbVolumeTimestep.localLoading = g_localLoading;
          vdbVolumeTimestep.variableNum = i;

          if (!g_localLoading) {
            vdbVolumeTimestep.queueGenerateVolumeData();
            vdbVolumeTimestep.waitGenerateVolumeData();
          }

          vol = vdbVolumeTimestep.createSGVolume();
          vol->child("anisotropy").setValue(0.875f);
          vol->child("densityScale").setValue(1.f);

        } else {
          if (dimensions.x == -1 || gridSpacing.x == -1) {
            throw std::runtime_error(
                "improper dimensions or grid spacing specified for volume");
          }
          if (voxelType == 0)
            throw std::runtime_error("improper voxelType specified for volume");

          auto volumeTimestep = VolumeTimestep(allVariablesData[i][f],
              voxelType,
              dimensions,
              gridOrigin,
              gridSpacing);
          volumeTimestep.localLoading = g_localLoading;
          volumeTimestep.variableNum = i;

          if (!g_localLoading) {
            volumeTimestep.queueGenerateVolumeData();
            volumeTimestep.waitGenerateVolumeData();
          }

          vol = volumeTimestep.createSGVolume();
        }

        auto tfn = std::static_pointer_cast<sg::TransferFunction>(
            sg::createNode("tfn_" + to_string(i), "transfer_function_turbo"));

        for (int j = 0; j < numInstances; j++) {
          auto newX = createNode("geomXfm" + to_string(j), "transform");
          newX->child("translation") = vec3f(j + 20 * j + i, 0, 0);
          newX->add(vol);
          tfn->add(newX);
        }

        world->add(tfn);
        world->render();
      }
      g_allSeparateWorlds.push_back(singleVariableWorlds);
    }
  }

  if (importAsSeparateTimeseries) {
    arcballCamera.reset(
        new ArcballCamera(g_allSeparateWorlds[0][0]->bounds(), windowSize));
  } else {
    arcballCamera.reset(
        new ArcballCamera(g_allWorlds[0]->bounds(), windowSize));
  }
  activeWindow->updateCamera();

  // set initial timestep
  if (importAsSeparateTimeseries)
    setVariableTimeseries(0, 0);
  else
    setTimestep(0);

  activeWindow->registerImGuiCallback([&]() { addTimeseriesUI(); });

  activeWindow->registerDisplayCallback(
      [&](MainWindow *) { animateTimesteps(); });

  activeWindow->mainLoop();
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

void TimeSeriesWindow::addToCommandLine(std::shared_ptr<CLI::App> app) {
  app->add_option(
    "--lightType",
    lightTypeStr,
    "set the type of light"
  )->check(CLI::IsMember({"ambient", "distant", "hdri", "sphere", "spot", "sunSky", "quad"}));
  app->add_option(
    "--numInstances",
    numInstances,
    "set the number of rendering instances"
  )->check(CLI::PositiveNumber);

  app->add_flag(
    "--separateTimeseries",
    importAsSeparateTimeseries,
    "Load volumes as separate timeseries objects"
  );
  app->add_flag(
    "--localLoading",
    g_localLoading,
    "Load volumes locally"
  );
  app->add_flag(
    "--separateFb",
    setSeparateFramebuffers,
    "Render into separate framebuffers"
  );

  app->remove_option(app->get_option_no_throw("files"));
  app->add_option_function<std::vector<std::string>>(
    "files",
    [&](const std::vector<std::string> val) {
      // fit it into a unit cube (if no other gridSpacing provided)
      if (gridSpacing == vec3f(-1.f)) {
        const float normalizedGridSpacing = reduce_min(1.f / dimensions);

        gridOrigin  = vec3f(-0.5f * dimensions * normalizedGridSpacing);
        gridSpacing = vec3f(normalizedGridSpacing);
      }

      allVariablesData.push_back(val);
      return true;
    },
    "Load these volume files"
  );
}

bool TimeSeriesWindow::parseCommandLine()
{
  int ac = studioCommon.argc;
  const char **av = studioCommon.argv;

  std::shared_ptr<CLI::App> app = std::make_shared<CLI::App>("OSPRay Studio Timeseries");
  StudioContext::addToCommandLine(app);
  MainWindow::addToCommandLine(app);
  TimeSeriesWindow::addToCommandLine(app);
  try {
    app->parse(ac, av);
  } catch (const CLI::ParseError &e) {
    exit(app->exit(e));
  }

  std::cerr << "local loading for time steps (on each process): "
            << g_localLoading << std::endl;

  return 1;
}

void TimeSeriesWindow::addTimeseriesUI()
{
  auto frame = activeWindow->getFrame();
  ImGui::Begin("Time series");

  int numTimesteps;

  if(importAsSeparateTimeseries) {
    if (ImGui::Combo("variableTimeseries##whichVariable",
                     &whichVariable,
                     variableUI_callback,
                     nullptr,
                     g_allSeparateWorlds.size())) {
      frame->cancelFrame();
      setVariableTimeseries(whichVariable, 0);
    }
    numTimesteps = g_allSeparateWorlds[whichVariable].size();
  } else {
    numTimesteps = g_allWorlds.size();
  }
  setTimestepsUI(numTimesteps);
}

void TimeSeriesWindow::setTimestepsUI(int numTimesteps){

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
    if (importAsSeparateTimeseries)
      setVariableTimeseries(whichVariable,
                            g_timeseriesParameters.currentTimestep);
    else
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
    if (activeWindow) {
      frame->pauseRendering = 
          g_timeseriesParameters.pauseRendering;
    }
  }

  ImGui::End();
}

void TimeSeriesWindow::animateTimesteps()
{
  int numTimesteps = importAsSeparateTimeseries ? g_allSeparateWorlds[whichVariable].size() : g_allWorlds.size();

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
      if (importAsSeparateTimeseries)
        setVariableTimeseries(whichVariable,
                              g_timeseriesParameters.currentTimestep);
      else

        setTimestep(g_timeseriesParameters.currentTimestep);
      timestepLastChanged = now;
    }
  }
}

void TimeSeriesWindow::setTimestepFb(int timestep)
{
  auto frame = activeWindow->getFrame();
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
  auto frame               = activeWindow->getFrame();
  framebufferResetRequired = rkcommon::utility::TimeStamp();

  if (frame->hasChild("framebuffer")) {
    auto &framebuffer = frame->childAs<FrameBuffer>("framebuffer");

    // reset accumulation for current frame buffer only
    framebuffer.resetAccumulation();
    framebufferLastReset[currentTimestep] = framebufferResetRequired;
  }
}

void TimeSeriesWindow::setVariableTimeseries(int whichVariable, int timestep)
{
  auto frame = activeWindow->getFrame();
  auto world = g_allSeparateWorlds[whichVariable][timestep];

  // Simply changing the world doesn't mark lights or world as modified.  Make
  // sure the current lights list is set on the new world.
  lightsManager->updateWorld(*world);
  frame->add(world);

  //set up separate framebuffers
  if (setSeparateFramebuffers)
    setTimestepFb(timestep);
}

void TimeSeriesWindow::setTimestep(int timestep)
{
  auto frame = activeWindow->getFrame();
  auto world = g_allWorlds[timestep];

  // Simply changing the world doesn't mark lights or world as modified.  Make
  // sure the current lights list is set on the new world.
  lightsManager->updateWorld(*world);
  frame->add(world); 

  //set up separate framebuffers
  if (setSeparateFramebuffers)
    setTimestepFb(timestep);
}
