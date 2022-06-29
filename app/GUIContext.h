// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// GUIContext class is specialisation of StudioContext class for OSPRay Studio
// GUI mode; this class implements all base class functions and has extra
// functions for GUI mode functionality

#pragma once

#include "ospStudio.h"
// ospray sg
#include "sg/Frame.h"
// ospray widgets
#include "PluginManager.h"
#include "widgets/Panel.h"

#include "rkcommon/utility/getEnvVar.h"
// json
#include "sg/JSONDefs.h"

using namespace rkcommon::math;
using namespace ospray;
using rkcommon::make_unique;

class MainWindow;
class WindowsBuilder;
class MainMenuBuilder;
class GUIContext : public StudioContext
{
 public:
  GUIContext(StudioCommon &studioCommon);
  ~GUIContext();

  ///////////////////////////////////////////////////////////////
  // common studio context function overrides
  void addToCommandLine(std::shared_ptr<CLI::App> app) override;
  bool parseCommandLine() override;
  void start() override;
  void importFiles(sg::NodePtr world) override;
  void refreshRenderer();
  void updateCamera() override;
  void refreshScene(bool resetCamera) override;

  //////////////////////////////////////////////////////////////
  // GUI-context extras
  void saveRendererParams();
  void changeToDefaultCamera();
  void printUtilNode(const std::string &utilNode);
  bool resHasHit(float &x, float &y, vec3f &worldPosition);
  void getWindowTitle(std::stringstream &windowTitle);
  vec2i &getFBSize();
  void updateFovy(const float &sensitivity, const float &scrollY);
  void useSceneCamera();
  bool g_animatingPath = false;

  void saveCurrentFrame();
  void saveSGScene();
  void saveNodesJson(const std::string nodeTypeStr);

  sg::NodePtr g_selectedSceneCamera;

  // Plugins //
  std::vector<std::unique_ptr<Panel>> pluginPanels;

  rkcommon::FileName backPlateTexture = "";

  // CLI
  bool optShowColor{true};
  bool optShowAlbedo{false};
  bool optShowDepth{false};
  bool optShowDepthInvert{false};
  bool optAutorotate{false};
  bool optAnimate{false};

  std::shared_ptr<std::vector<CameraState>> cameraStack;

  static MainWindow *mainWindow;
  
  std::shared_ptr<sg::FrameBuffer> framebuffer = nullptr;
  std::shared_ptr<GUIContext> currentUtil = nullptr;
  vec2i windowSize;

  // list of cameras imported with the scene definition
  static CameraMap g_sceneCameras;

  // windows and main menu builder related functions
  void selectBuffer(int whichBuffer);
  void addCameraState();
  void removeCameraState();
  void viewCameraPath(bool showCameraPath);

  void setCameraSnapshot(size_t snapshot);
  void selectCamStackIndex(size_t i);

  void selectCamera(size_t whichCamera);
  void createNewCamera(const std::string newType);
  void removeLight(int whichLight);
  void createIsoSurface(
      int currentVolume, std::vector<ospray::sg::NodePtr> &volumes);
  void clearScene();
  void renderTexturedQuad(vec2f &border);
  void setLockUpDir(const vec3f &lockUpDir);
  void setUpDir(const vec3f &upDir);
  void animationSetShowUI();
  void quitNextFrame();

  float g_camPathSpeed = 5; // defined in hundredths (e.g. 10 = 10 * 0.01 = 0.1)
  int g_camSelectedStackIndex = 0;
  int g_camCurrentPathIndex = 0;

  // FPS measurement of last frame
  float latestFPS{0.f};
  float lockAspectRatio = 0.0;
  // Option to always show a gamma corrected display to user.  Native sRGB
  // buffer is untouched, linear buffers are displayed as sRGB.
  bool uiDisplays_sRGB{true};

  // Camera motion controls
  float maxMoveSpeed{1.f};
  float fineControl{0.2f};
  float preFPVZoom{0.f};

  // auto rotation speed, 1=0.1% window width mouse movement, 100=10%
  int autorotateSpeed{1};

  std::string scene;
};
