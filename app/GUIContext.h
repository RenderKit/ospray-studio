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

#include "CameraStack.h"

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

  void loadCamJson() override;
  sg::NodePtr g_selectedSceneCamera;

  // Plugins //
  std::vector<std::unique_ptr<Panel>> pluginPanels;

  rkcommon::FileName backPlateTexture = "";

  // CLI
  bool optSaveImageOnGUIExit{false};

  uint32_t optDisplayBuffer{OSP_FB_COLOR}; // OSPFrameBufferChannel 
  bool optDisplayBufferInvert{false};
  bool optAutorotate{false};
  bool optAnimate{false};
  std::string optDisplayJsonName{""};

  static MainWindow *mainWindow;
  
  std::shared_ptr<sg::FrameBuffer> framebuffer = nullptr;

  vec2i defaultSize;

  // windows and main menu builder related functions
  void selectBuffer(OSPFrameBufferChannel whichBuffer, bool invert = false);

  void selectCamera() override;
  void createNewCamera(const std::string newType);
  void createIsoSurface(
      int currentVolume, std::vector<ospray::sg::NodePtr> &volumes);
  void clearScene();

  std::string scene;
  
  float lockAspectRatio = 0.0;
};
