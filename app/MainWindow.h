// Copyright 2018-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ospStudio.h"

#include "ArcballCamera.h"
// glfw
#include "GLFW/glfw3.h"
// ospray sg
#include "sg/Frame.h"
#include "sg/renderer/MaterialRegistry.h"
// std
#include <functional>

#include <map>
#include "widgets/AnimationWidget.h"
#include "PluginManager.h"
#include "sg/importer/Importer.h"

using namespace rkcommon::math;
using namespace ospray;
using rkcommon::make_unique;

// on Windows often only GL 1.1 headers are present
// and Mac may be missing the float defines
#ifndef GL_CLAMP_TO_BORDER
#define GL_CLAMP_TO_BORDER 0x812D
#endif
#ifndef GL_FRAMEBUFFER_SRGB
#define GL_FRAMEBUFFER_SRGB 0x8DB9
#endif
#ifndef GL_RGBA32F
#define GL_RGBA32F 0x8814
#endif
#ifndef GL_RGB32F
#define GL_RGB32F 0x8815
#endif

enum class OSPRayRendererType
{
  SCIVIS,
  PATHTRACER,
  AO,
  DEBUGGER,
  OTHER
};

static std::map<std::string, const char *> const guiCommandLineAliases = {
  {"-pf", "--pixelfilter"},
  {"-a", "--animate"},
  {"-dim", "--dimensions"},
  {"-gs", "--gridSpacing"},
  {"-go", "--gridOrigin"},
  {"-vt", "--voxelType"},
  {"-sc", "--sceneConfig"},
  {"-ic", "--instanceConfig"},
  {"-ps", "--pointSize"},
};

class MainWindow : public StudioContext
{
 public:
 
  MainWindow(StudioCommon &studioCommon);

  ~MainWindow();

  static MainWindow *getActiveWindow();

  GLFWwindow* getGLFWWindow();

  void registerDisplayCallback(std::function<void(MainWindow *)> callback);

  void registerKeyCallback(
      std::function<void(
          MainWindow *, int key, int scancode, int action, int mods)>);

  void registerImGuiCallback(std::function<void()> callback);

  void mainLoop();

  void addToCommandLine(std::shared_ptr<CLI::App> app) override;
  bool parseCommandLine() override;

  void start() override;

  void importFiles(sg::NodePtr world) override;

  std::shared_ptr<sg::Frame> getFrame();

  bool timeseriesMode = false;

  std::stringstream windowTitle;

  void updateTitleBar();

  void refreshRenderer();

  void updateCamera() override;
  void setCameraState(CameraState &cs) override;
  void refreshScene(bool resetCamera) override;
  int whichLightType{-1};
  int whichCamera{0};
  std::string lightTypeStr{"ambient"};
  std::string scene;
  std::string rendererTypeStr;

 protected:
  void buildPanel();
  void reshape(const vec2i &newWindowSize);
  void motion(const vec2f &position);
  void keyboardMotion();
  void mouseButton(const vec2f &position);
  void mouseWheel(const vec2f &scroll);
  void display();
  void startNewOSPRayFrame();
  void waitOnOSPRayFrame();
  void buildUI();
  void addLight();
  void removeLight();
  void addPTMaterials();

  void saveCurrentFrame();
  void centerOnEyePos();
  void pickCenterOfRotation(float x, float y);
  void pushLookMark();
  void popLookMark();

  // menu and window UI
  void buildMainMenu();
  void buildMainMenuFile();
  void buildMainMenuEdit();
  void buildMainMenuView();
  void buildMainMenuPlugins();

  void buildWindows();
  void buildWindowRendererEditor();
  void buildWindowFrameBufferEditor();
  void buildWindowKeyframes();
  void buildWindowSnapshots();
  void buildWindowLightEditor();
  void buildWindowCameraEditor();
  void buildWindowMaterialEditor();
  void buildWindowTransferFunctionEditor();
  void buildWindowIsosurfaceEditor();
  void buildWindowTransformEditor();
  void buildWindowRenderingStats();

  void setCameraSnapshot(size_t snapshot);
  void printHelp() override;

  std::vector<CameraState> cameraStack;
  sg::NodePtr g_selectedSceneCamera;

  // Plugins //
  std::vector<std::unique_ptr<Panel>> pluginPanels;

  // imgui window visibility toggles
  bool showRendererEditor{false};
  bool showFrameBufferEditor{false};
  bool showKeyframes{false};
  bool showSnapshots{false};
  bool showLightEditor{false};
  bool showCameraEditor{false};
  bool showMaterialEditor{false};
  bool showTransferFunctionEditor{false};
  bool showIsosurfaceEditor{false};
  bool showTransformEditor{false};
  bool showRenderingStats{false};

  // imgui-controlled options
  std::string screenshotFiletype{"png"};
  bool screenshotAlbedo{false};
  bool screenshotDepth{false};
  bool screenshotNormal{false};
  bool screenshotLayers{false};

  // Option to always show a gamma corrected display to user.  Native sRGB
  // buffer is untouched, linear buffers are displayed as sRGB.
  bool uiDisplays_sRGB{true}; 

  static MainWindow *activeWindow;

  vec2i windowSize;
  vec2i fbSize;
  vec2f previousMouse{-1.f};

  OSPRayRendererType rendererType{OSPRayRendererType::SCIVIS};
  int optPF = -1; // optional pixel filter, -1 = use default

  rkcommon::FileName backPlateTexture = "";

  // GLFW window instance
  GLFWwindow *glfwWindow = nullptr;

  // OpenGL framebuffer texture
  GLuint framebufferTexture = 0;

  // optional registered display callback, called before every display()
  std::function<void(MainWindow *)> displayCallback;

  // toggles display of ImGui UI, if an ImGui callback is provided
  bool showUi = true;

  // optional registered ImGui callback, called during every frame to build UI
  std::function<void()> uiCallback;

  // optional registered key callback, called when keys are pressed
  std::function<void(
      MainWindow *, int key, int scancode, int action, int mods)>
      keyCallback;

  // FPS measurement of last frame
  float latestFPS{0.f};

  // auto rotation speed, 1=0.1% window width mouse movement, 100=10%
  int autorotateSpeed{1};

  // Camera motion controls
  float maxMoveSpeed{1.f};
  float fineControl{0.2f};
  float preFPVZoom{0.f};
  affine3f lastCamXfm{one};

  // format used by glTexImage2D, as determined at context creation time
  GLenum gl_rgb_format;
  GLenum gl_rgba_format;

  std::shared_ptr<AnimationWidget> animationWidget{nullptr};

  // CLI
  bool optShowColor{true};
  bool optShowAlbedo{false};
  bool optShowDepth{false};
  bool optShowDepthInvert{false};
  bool optAutorotate{false};
  bool optAnimate{false};
};
