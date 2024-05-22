// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

// std
#include <iostream>
#include <map>
#include <stdexcept>
#include <vector>
// ospray
#include "ospray/ospray_util.h"
// ospray sg
#include "sg/Frame.h"
#include "sg/Scheduler.h"
#include "sg/renderer/MaterialRegistry.h"
#include "sg/scene/lights/LightsManager.h"
#include "sg/Mpi.h"
// studio app
#include "AnimationManager.h"
// ospcommon
#include "rkcommon/common.h"
#include "rkcommon/utility/getEnvVar.h"

#include "version.h"

// Forward-declare CLI::App to prevent every file that imports this header from
// also having to load CLI11.hpp.
namespace CLI {
class App;
}

using namespace ospray;
using namespace rkcommon;

class PluginManager;
enum class StudioMode
{
  GUI,
  BATCH,
  HEADLESS,
  BENCHMARK,
};

enum class OSPRayRendererType
{
  SCIVIS,
  PATHTRACER,
  AO,
  DEBUGGER,
#ifdef USE_MPI
  MPIRAYCAST,
#endif
  OTHER
};

const static std::map<std::string, StudioMode> StudioModeMap = {
    {"gui", StudioMode::GUI},
    {"batch", StudioMode::BATCH},
    {"server", StudioMode::HEADLESS},
    {"benchmark", StudioMode::BENCHMARK},
};

const static std::map<std::string, vec2i> standardResolutionSizeMap = {
    {"144p", {256, 144}},
    {"240p", {426, 240}},
    {"360p", {640, 360}},
    {"480p", {640, 480}},
    {"720p", {1280, 720}},
    {"1080p", {1920, 1080}},
    {"1440p", {2560, 1440}},
    {"2160p", {3840, 2160}},
    {"4k", {3840, 2160}},
    {"4320p", {7680, 4320}},
    {"8k", {7680, 4320}}};

// Common across all modes

class StudioCommon
{
 public:
  StudioCommon(std::vector<std::string> _pluginsToLoad,
      const bool denoiser,
      int _argc,
      const char **_argv)
      : pluginsToLoad(_pluginsToLoad),
        denoiserAvailable(denoiser),
        argc(_argc),
        argv(_argv)
  {}

  void splitPluginArguments();

  std::vector<std::string> pluginsToLoad;
  bool denoiserAvailable{false};
  const vec2i defaultSize = vec2i(1024, 768);

  int argc;
  const char **argv;
  int plugin_argc{0};
  const char **plugin_argv{nullptr};
};

using CameraMap = containers::FlatMap<std::string, sg::NodePtr>;

// abstract base class for all Studio modes
// XXX: should be merged with StudioCommon above
class StudioContext : public std::enable_shared_from_this<StudioContext>
{
 public:
  StudioContext(StudioCommon &_common, StudioMode _mode) : studioCommon(_common)
  {
    frame = sg::createNodeAs<sg::Frame>("main_frame", "frame");

    // baseMaterialRegistry and lightsManager are owned by the Frame
    baseMaterialRegistry = frame->baseMaterialRegistry;
    lightsManager = frame->lightsManager;
    mode = _mode;
    optResolution = _common.defaultSize;
    scheduler = sg::Scheduler::create();
    animationManager = std::make_shared<AnimationManager>();
  }

  virtual ~StudioContext() {}

  virtual void start() = 0;
  virtual bool parseCommandLine() = 0;
  virtual void addToCommandLine(std::shared_ptr<CLI::App> app);
  virtual void importFiles(sg::NodePtr world) = 0;
  virtual void refreshScene(bool resetCam) = 0;
  virtual void updateCamera() = 0;
  virtual void selectCamera() = 0;
  virtual void updateCameraIndices(uint32_t idx);
  virtual void loadCamJson() = 0;
  
  std::shared_ptr<sg::Frame> frame;
  std::shared_ptr<sg::MaterialRegistry> baseMaterialRegistry;
  std::shared_ptr<sg::LightsManager> lightsManager;
  std::shared_ptr<AnimationManager> animationManager{nullptr};
  std::shared_ptr<PluginManager> pluginManager;
  std::shared_ptr<sg::Scheduler> scheduler;

  std::vector<std::string> filesToImport;

  // global context camera settings for loading external cameras
  std::shared_ptr<affine3f> cameraView{nullptr};
  int cameraIdx{0};
  int cameraSettingsIdx{0};
  std::shared_ptr<CameraMap> sgFileCameras{nullptr};

  int defaultMaterialIdx = 0;

  std::string outputFilename{""};

  StudioMode mode;
  void *ctxMainWindow{nullptr}; // Will be populated if in GUI mode
  void setMainWindow(void *mainWindow) {
    ctxMainWindow = mainWindow;
  };
  void *getMainWindow() {
    return ctxMainWindow;
  };

  bool optReloadAssets{false};
  bool optResetCameraOnLoad{true};
  std::string optRendererTypeStr{"pathtracer"};
  std::string optCameraTypeStr{"perspective"};
  bool optVerboseImporter{false};
  int optSPP{32};
  float optVariance{0.f}; // varianceThreshold
  sg::rgba optBackGroundColor{vec3f(0.0f), 1.f}; // default to black
  OSPPixelFilterType optPF{OSP_PIXELFILTER_GAUSS};
  bool optDenoiser{false};
  bool optDenoiseFinalFrame{false};
  bool optGridEnable{false};
  vec3i optGridSize{1, 1, 1};
  OSPStereoMode optStereoMode{OSP_STEREO_NONE};
  float optInterpupillaryDistance{0.0635f};
  sg::NodePtr volumeParams{};
  float pointSize{0.05f};
  vec2i optResolution{0, 0};
  std::string optSceneConfig{""};
  std::string optInstanceConfig{""};
  bool optDoAsyncTasking{false};
  float maxContribution{math::inf};
  int frameAccumLimit{0};
  std::string optImageName{"studio"}; // (each mode sets this default)
  std::string optImageFormat{"png"};
  std::string optCamJsonName{""};
  bool optSaveAlbedo{false};
  bool optSaveDepth{false};
  bool optSaveNormal{false};
  bool optSaveLayersSeparately{false};
  range1i optCameraRange{0, 0};

  StudioCommon &studioCommon;

  bool showPoseBBox{false};
  bool showInstBBox{false};
  bool showInstBBoxFrame{false};

  // expose scene cameras for plugins:
  // list of cameras imported with the scene definition
  CameraMap g_sceneCameras;
  int whichCamera{0};

 protected:
  virtual box3f getSceneBounds();

  bool sgScene{false}; // whether we are loading a scene file
};

inline OSPError initializeOSPRay(int &argc, const char **argv, bool use_mpi)
{
  if (!use_mpi) {
    // initialize OSPRay; OSPRay parses (and removes) its commandline
    // parameters, e.g. "--osp:debug"
    OSPError initError = ospInit(&argc, argv);

    if (initError != OSP_NO_ERROR) {
      std::cerr << "OSPRay not initialized correctly!" << std::endl;
      return initError;
    }

    OSPDevice device(ospGetCurrentDevice());
    if (!device) {
      std::cerr << "OSPRay device could not be fetched!" << std::endl;
      return OSP_UNKNOWN_ERROR;
    }

    // TODO: setErrorCallback and ospDeviceSetParam calls seem to break mpi.
    // set an error callback to catch any OSPRay errors
    ospDeviceSetErrorCallback(
        device,
        [](void *, OSPError, const char *errorDetails) {
          std::cerr << "OSPRay error: " << errorDetails << std::endl;
        },
        nullptr);

    ospDeviceSetStatusCallback(
        device, [](void *, const char *msg) { std::cout << msg; }, nullptr);

    bool warnAsErrors = true;
    auto logLevel = OSP_LOG_WARNING;

    ospDeviceSetParam(device, "warnAsError", OSP_BOOL, &warnAsErrors);
    ospDeviceSetParam(device, "logLevel", OSP_INT, &logLevel);

    ospDeviceCommit(device);
    ospDeviceRelease(device);

  } else {
    // Initialize MPI and set rank and world size in sg
    sg::sgInitializeMPI(argc, argv);
    std::cout << "ospStudio --mpi, rank " << sg::sgMpiRank() << "/"
              << sg::sgMpiWorldSize() << "\n";

    OSPError initError = ospInit(&argc, argv);

    if (initError != OSP_NO_ERROR) {
      std::cerr << "OSPRay not initialized correctly!" << std::endl;
      return initError;
    }

    cpp::Device mpiDevice = cpp::Device::current();
    mpiDevice.commit();
    mpiDevice.setCurrent();

    // set an error callback to catch any OSPRay errors and exit the application
    ospDeviceSetErrorCallback(
        mpiDevice.handle(),
        [](void *, OSPError error, const char *errorDetails) {
          std::cerr << "OSPRay error: " << errorDetails << std::endl;
          exit(error);
        },
        nullptr);
  }

  return OSP_NO_ERROR;
}
