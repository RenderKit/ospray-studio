// Copyright 2018-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "MainWindow.h"
// imgui
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl2.h"
#include "Proggy.h"
// std
#include <chrono>
#include <iostream>
#include <stdexcept>
// ospray_sg
#include "sg/camera/Camera.h"
#include "sg/exporter/Exporter.h"
#include "sg/fb/FrameBuffer.h"
#include "sg/generator/Generator.h"
#include "sg/renderer/Renderer.h"
#include "sg/scene/World.h"
#include "sg/scene/lights/LightsManager.h"
#include "sg/visitors/Commit.h"
#include "sg/visitors/GenerateImGuiWidgets.h"
#include "sg/visitors/PrintNodes.h"
#include "sg/visitors/Search.h"
#include "sg/visitors/SetParamByNode.h"
#include "sg/visitors/CollectTransferFunctions.h"
#include "sg/scene/volume/Volume.h"
// rkcommon
#include "rkcommon/math/rkmath.h"
#include "rkcommon/os/FileName.h"
#include "rkcommon/utility/SaveImage.h"
#include "rkcommon/utility/getEnvVar.h"
#include "rkcommon/utility/DataView.h"

// json
#include "sg/JSONDefs.h"

#include <fstream>
#include <queue>
// widgets
#include "widgets/AdvancedMaterialEditor.h"
#include "widgets/FileBrowserWidget.h"
#include "widgets/ListBoxWidget.h"
#include "widgets/SearchWidget.h"
#include "widgets/TransferFunctionWidget.h"
#include "widgets/PieMenu.h"
#include "widgets/Guizmo.h"

// CLI
#include <CLI11.hpp>

using namespace ospray_studio;
using namespace ospray;

static ImGuiWindowFlags g_imguiWindowFlags = ImGuiWindowFlags_AlwaysAutoResize;

static bool g_quitNextFrame = false;
static bool g_saveNextFrame = false;
static bool g_animatingPath = false;
static bool g_clearSceneConfirm = false;

static const std::vector<std::string> g_scenes = {"tutorial_scene",
    "sphere",
    "particle_volume",
    "random_spheres",
    "wavelet",
    "wavelet_slices",
    "torus_volume",
    "unstructured_volume",
    "multilevel_hierarchy"};

#ifdef USE_MPI
static const std::vector<std::string> g_renderers = {
    "scivis", "pathtracer", "ao", "debug", "mpiRaycast"};
#else
static const std::vector<std::string> g_renderers = {
    "scivis", "pathtracer", "ao", "debug"};
#endif

// list of cameras imported with the scene definition
static rkcommon::containers::FlatMap<std::string, sg::NodePtr> g_sceneCameras;

static const std::vector<std::string> g_debugRendererTypes = {"eyeLight",
    "primID",
    "geomID",
    "instID",
    "Ng",
    "Ns",
    "backfacing_Ng",
    "backfacing_Ns",
    "dPds",
    "dPdt",
    "volume"};

static const std::vector<std::string> g_lightTypes = {
    "ambient", "distant", "hdri", "sphere", "spot", "sunSky", "quad"};

std::vector<CameraState> g_camPath; // interpolated path through cameraStack
int g_camSelectedStackIndex = 0;
int g_camCurrentPathIndex = 0;
float g_camPathSpeed = 5; // defined in hundredths (e.g. 10 = 10 * 0.01 = 0.1)
const int g_camPathPause = 2; // _seconds_ to pause for at end of path
int g_rotationConstraint = -1;

const double CAM_MOVERATE =
    10.0; // TODO: the constant should be scene dependent or user changeable
double g_camMoveX = 0.0;
double g_camMoveY = 0.0;
double g_camMoveZ = 0.0;
double g_camMoveA = 0.0;
double g_camMoveE = 0.0;
double g_camMoveR = 0.0;

float lockAspectRatio = 0.0;

sg::NodePtr g_copiedMat = nullptr;

std::string quatToString(quaternionf &q)
{
  std::stringstream ss;
  ss << q;
  return ss.str();
}

bool rendererUI_callback(void *, int index, const char **out_text)
{
  *out_text = g_renderers[index].c_str();
  return true;
}

bool debugTypeUI_callback(void *, int index, const char **out_text)
{
  *out_text = g_debugRendererTypes[index].c_str();
  return true;
}

bool lightTypeUI_callback(void *, int index, const char **out_text)
{
  *out_text = g_lightTypes[index].c_str();
  return true;
}

bool cameraUI_callback(void *, int index, const char **out_text)
{
  *out_text = g_sceneCameras.at_index(index).first.c_str();
  return true;
}

bool stringVec_callback(void *data, int index, const char **out_text)
{
  *out_text = ((std::string *)data)[index].c_str();
  return true;
}

std::string vec3fToString(const vec3f &v)
{
  std::stringstream ss;
  ss << v;
  return ss.str();
}

// MainWindow definitions ///////////////////////////////////////////////

void error_callback(int error, const char *desc)
{
  std::cerr << "error " << error << ": " << desc << std::endl;
}

MainWindow *MainWindow::activeWindow = nullptr;

MainWindow::MainWindow(StudioCommon &_common)
    : StudioContext(_common, StudioMode::GUI), windowSize(_common.defaultSize), scene("")
{
  pluginManager = std::make_shared<PluginManager>();
  if (activeWindow != nullptr) {
    throw std::runtime_error("Cannot create more than one MainWindow!");
  }

  optSPP = 1; // Default SamplesPerPixel in interactive mode is one.

  activeWindow = this;

  glfwSetErrorCallback(error_callback);

  // initialize GLFW
  if (!glfwInit()) {
    throw std::runtime_error("Failed to initialize GLFW!");
  }

  glfwWindowHint(GLFW_SRGB_CAPABLE, GLFW_TRUE);

  // get primary monitor's display scaling
  GLFWmonitor *primaryMonitor = glfwGetPrimaryMonitor();
  if (primaryMonitor)
    glfwGetMonitorContentScale(primaryMonitor, &contentScale.x, &contentScale.y);

  // create GLFW window
  glfwWindow = glfwCreateWindow(
      windowSize.x, windowSize.y, "OSPRay Studio", nullptr, nullptr);

  if (!glfwWindow) {
    glfwTerminate();
    throw std::runtime_error("Failed to create GLFW window!");
  }

  glfwMakeContextCurrent(glfwWindow);

  // Determine whether GL framebuffer has float format
  // and set format used by glTexImage2D correctly.
  if (glfwGetWindowAttrib(glfwWindow, GLFW_CONTEXT_VERSION_MAJOR) < 3) {
    gl_rgb_format = GL_RGB16;
    gl_rgba_format = GL_RGBA16;
  } else {
    gl_rgb_format = GL_RGB32F;
    gl_rgba_format = GL_RGBA32F;
  }

  glfwSetKeyCallback(
      glfwWindow, [](GLFWwindow *, int key, int, int action, int mod) {
        auto &io = ImGui::GetIO();
        if (!io.WantCaptureKeyboard)
          if (action == GLFW_PRESS) {
            switch (key) {
            case GLFW_KEY_UP:
              if (mod != GLFW_MOD_ALT)
                g_camMoveZ = -CAM_MOVERATE;
              else
                g_camMoveY = -CAM_MOVERATE;
              break;
            case GLFW_KEY_DOWN:
              if (mod != GLFW_MOD_ALT)
                g_camMoveZ = CAM_MOVERATE;
              else
                g_camMoveY = CAM_MOVERATE;
              break;
            case GLFW_KEY_LEFT:
              g_camMoveX = -CAM_MOVERATE;
              break;
            case GLFW_KEY_RIGHT:
              g_camMoveX = CAM_MOVERATE;
              break;
            case GLFW_KEY_W:
              g_camMoveE = CAM_MOVERATE;
              break;
            case GLFW_KEY_S:
              if (mod != GLFW_MOD_CONTROL)
                g_camMoveE = -CAM_MOVERATE;
              else
                g_saveNextFrame = true;
              break;
            case GLFW_KEY_A:
              if (mod != GLFW_MOD_ALT)
                g_camMoveA = -CAM_MOVERATE;
              else
                g_camMoveR = -CAM_MOVERATE;
              break;
            case GLFW_KEY_D:
              if (mod != GLFW_MOD_ALT)
                g_camMoveA = CAM_MOVERATE;
              else
                g_camMoveR = CAM_MOVERATE;
              break;

            case GLFW_KEY_X:
              g_rotationConstraint = 0;
              break;
            case GLFW_KEY_Y:
              g_rotationConstraint = 1;
              break;
            case GLFW_KEY_Z:
              g_rotationConstraint = 2;
              break;

            case GLFW_KEY_I:
              activeWindow->centerOnEyePos();
              break;

            case GLFW_KEY_G:
              activeWindow->showUi = !(activeWindow->showUi);
              break;
            case GLFW_KEY_Q: {
              auto showMode =
                  rkcommon::utility::getEnvVar<int>("OSPSTUDIO_SHOW_MODE");
              // XXX Invoke the "Jim-Q" key, make it more difficult to exit
              // by mistake.
              if (showMode && mod != GLFW_MOD_CONTROL)
                std::cout << "Use ctrl-Q to exit\n";
              else
                g_quitNextFrame = true;
            } break;
            case GLFW_KEY_P:
              activeWindow->frame->traverse<sg::PrintNodes>();
              break;
            case GLFW_KEY_M:
              activeWindow->baseMaterialRegistry->traverse<sg::PrintNodes>();
              break;
            case GLFW_KEY_L:
              activeWindow->lightsManager->traverse<sg::PrintNodes>();
              break;
            case GLFW_KEY_B:
              PRINT(activeWindow->frame->bounds());
              break;
            case GLFW_KEY_V:
              activeWindow->frame->child("camera").traverse<sg::PrintNodes>();
              break;
            case GLFW_KEY_SPACE:
              if (activeWindow->cameraStack.size() >= 2) {
                g_animatingPath = !g_animatingPath;
                g_camCurrentPathIndex = 0;
                if (g_animatingPath) {
                  g_camPath = buildPath(
                      activeWindow->cameraStack, g_camPathSpeed * 0.01);
                }
              }
              break;
            case GLFW_KEY_EQUAL:
              activeWindow->pushLookMark();
              break;
            case GLFW_KEY_MINUS:
              activeWindow->popLookMark();
              break;
            case GLFW_KEY_0: /* fallthrough */
            case GLFW_KEY_1: /* fallthrough */
            case GLFW_KEY_2: /* fallthrough */
            case GLFW_KEY_3: /* fallthrough */
            case GLFW_KEY_4: /* fallthrough */
            case GLFW_KEY_5: /* fallthrough */
            case GLFW_KEY_6: /* fallthrough */
            case GLFW_KEY_7: /* fallthrough */
            case GLFW_KEY_8: /* fallthrough */
            case GLFW_KEY_9:
              activeWindow->setCameraSnapshot((key + 9 - GLFW_KEY_0) % 10);
              break;
            }
          }
        if (action == GLFW_RELEASE) {
          switch (key) {
          case GLFW_KEY_X:
          case GLFW_KEY_Y:
          case GLFW_KEY_Z:
            g_rotationConstraint = -1;
            break;
          case GLFW_KEY_UP:
          case GLFW_KEY_DOWN:
            g_camMoveZ = 0;
            g_camMoveY = 0;
            break;
          case GLFW_KEY_LEFT:
          case GLFW_KEY_RIGHT:
            g_camMoveX = 0;
            break;
          case GLFW_KEY_W:
          case GLFW_KEY_S:
            g_camMoveE = 0;
            break;
          case GLFW_KEY_A:
          case GLFW_KEY_D:
            g_camMoveA = 0;
            g_camMoveR = 0;
            break;
          }
        }
      });

  // set GLFW callbacks
  glfwSetFramebufferSizeCallback(
      glfwWindow, [](GLFWwindow *, int newWidth, int newHeight) {
        activeWindow->reshape(vec2i{newWidth, newHeight});
      });

  glfwSetMouseButtonCallback(glfwWindow, [](GLFWwindow *win, int, int action, int) {
    ImGuiIO &io = ImGui::GetIO();
    if (!activeWindow->showUi || !io.WantCaptureMouse) {
      double x, y;
      glfwGetCursorPos(win, &x, &y);
      activeWindow->mouseButton(vec2f{float(x), float(y)});

      activeWindow->getFrame()->setNavMode(action == GLFW_PRESS);
    }
  });

  glfwSetScrollCallback(glfwWindow, [](GLFWwindow *, double x, double y) {
    ImGuiIO &io = ImGui::GetIO();
    if (!activeWindow->showUi || !io.WantCaptureMouse) {
      activeWindow->mouseWheel(vec2f{float(x), float(y)});
    }
  });

  glfwSetCursorPosCallback(glfwWindow, [](GLFWwindow *, double x, double y) {
    ImGuiIO &io = ImGui::GetIO();
    if (!activeWindow->showUi || !io.WantCaptureMouse) {
      activeWindow->motion(vec2f{float(x), float(y)});
    }
  });

  ImGui::CreateContext();

  // Enable context for ImGui experimental viewports
  ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

  ImGui_ImplGlfw_InitForOpenGL(glfwWindow, true);
  ImGui_ImplOpenGL2_Init();

  // Disable active viewports until users enables toggled in view menu
  ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_ViewportsEnable;

  // set ImGui font, scaled to display DPI
  auto &io = ImGui::GetIO();
  auto scaleFactor = std::max(contentScale.x, contentScale.y);
  auto scaledFontSize = fontSize * scaleFactor;
  ImVec2 imScale(contentScale.x, contentScale.y);

  ImFont *font = io.Fonts->AddFontFromMemoryCompressedTTF(
      ProggyClean_compressed_data, ProggyClean_compressed_size, scaledFontSize);
  io.FontGlobalScale = 1.f / scaleFactor;
  io.DisplayFramebufferScale = imScale;

  // set initial OpenGL state
  glEnable(GL_TEXTURE_2D);
  glDisable(GL_LIGHTING);

  // create OpenGL frame buffer texture
  glGenTextures(1, &framebufferTexture);
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, framebufferTexture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  refreshScene(true);

  // trigger window reshape events with current window size
  glfwGetFramebufferSize(glfwWindow, &windowSize.x, &windowSize.y);
  reshape(windowSize);
}

MainWindow::~MainWindow()
{
  ImGui_ImplOpenGL2_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
  glfwTerminate();
  pluginManager->removeAllPlugins();
  g_sceneCameras.clear();
  g_copiedMat = nullptr;
  sg::clearAssets();
}

void MainWindow::start()
{
  std::cerr << "GUI mode\n";

  // load plugins //
  for (auto &p : studioCommon.pluginsToLoad)
    pluginManager->loadPlugin(p);

  // create panels //
  // doing this outside constructor to ensure shared_from_this()
  // can wrap a valid weak_ptr (in constructor, not guaranteed)

  pluginManager->main(shared_from_this(), &pluginPanels);
  // std::move(newPluginPanels.begin(),
  //     newPluginPanels.end(),
  //     std::back_inserter(pluginPanels));

  std::ifstream cams("cams.json");
  if (cams) {
    JSON j;
    cams >> j;
    cameraStack = j.get<std::vector<CameraState>>();
  }

  if (parseCommandLine()) {
    refreshRenderer();
    mainLoop();
  }
}

MainWindow *MainWindow::getActiveWindow()
{
  return activeWindow;
}

std::shared_ptr<sg::Frame> MainWindow::getFrame()
{
  return frame;
}

void MainWindow::registerDisplayCallback(
    std::function<void(MainWindow *)> callback)
{
  displayCallback = callback;
}

void MainWindow::registerKeyCallback(std::function<void(
        MainWindow *, int key, int scancode, int action, int mods)> callback)
{
  keyCallback = callback;
}

void MainWindow::registerImGuiCallback(std::function<void()> callback)
{
  uiCallback = callback;
}

void MainWindow::mainLoop()
{
  // continue until the user closes the window
  startNewOSPRayFrame();

  while (!glfwWindowShouldClose(glfwWindow) && !g_quitNextFrame) {
    ImGui_ImplOpenGL2_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    display();

    glfwPollEvents();
  }

  waitOnOSPRayFrame();
}

void MainWindow::reshape(const vec2i &newWindowSize)
{
  windowSize = newWindowSize;
  vec2i fSize = windowSize;
  if (lockAspectRatio) {
    // Tell OSPRay to render the largest subset of the window that satisies the
    // aspect ratio
    float aspectCorrection = lockAspectRatio
        * static_cast<float>(newWindowSize.y)
        / static_cast<float>(newWindowSize.x);
    if (aspectCorrection > 1.f) {
      fSize.y /= aspectCorrection;
    } else {
      fSize.x *= aspectCorrection;
    }
  }
  if (frame->child("camera").hasChild("aspect"))
    frame->child("camera")["aspect"] = static_cast<float>(fSize.x) / fSize.y;

  frame->child("windowSize") = fSize;
  frame->currentAccum = 0;

  // reset OpenGL viewport and orthographic projection
  glViewport(0, 0, windowSize.x, windowSize.y);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0.0, windowSize.x, 0.0, windowSize.y, -1.0, 1.0);

  // update camera
  arcballCamera->updateWindowSize(windowSize);
}

void MainWindow::updateCamera()
{
  frame->currentAccum = 0;
  auto &camera = frame->child("camera");

  camera["position"] = arcballCamera->eyePos();
  camera["direction"] = arcballCamera->lookDir();
  camera["up"] = arcballCamera->upDir();

  if (camera.hasChild("focusDistance")) {
    float focusDistance = rkcommon::math::length(
        camera["lookAt"].valueAs<vec3f>() - arcballCamera->eyePos());
    if (camera["adjustAperture"].valueAs<bool>()) {
      float oldFocusDistance = camera["focusDistance"].valueAs<float>();
      if (!(isinf(oldFocusDistance) || isinf(focusDistance))) {
        float apertureRadius = camera["apertureRadius"].valueAs<float>();
        camera["apertureRadius"] = apertureRadius *
          focusDistance / oldFocusDistance;
      }
    }
    camera["focusDistance"] = focusDistance;
  }
}

void MainWindow::setCameraState(CameraState &cs)
{
  arcballCamera->setState(cs);
}

void MainWindow::centerOnEyePos()
{
  // Recenters camera at the eye position and zooms all the way in, like FPV
  // Save current zoom level
  preFPVZoom = arcballCamera->getZoomLevel();
  arcballCamera->setCenter(arcballCamera->eyePos());
  arcballCamera->setZoomLevel(0.f);
}

void MainWindow::pickCenterOfRotation(float x, float y)
{
  ospray::cpp::PickResult res;
  auto &fb = frame->childAs<sg::FrameBuffer>("framebuffer");
  auto &r = frame->childAs<sg::Renderer>("renderer");
  auto &c = frame->childAs<sg::Camera>("camera");
  auto &w = frame->childAs<sg::World>("world");

  x = clamp(x / windowSize.x, 0.f, 1.f);
  y = 1.f - clamp(y / windowSize.y, 0.f, 1.f);
  res = fb.handle().pick(r, c, w, x, y);
  if (res.hasHit) {
    if (!(glfwGetKey(glfwWindow, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)) {
      // Constraining rotation around the up works pretty well.
      arcballCamera->constrainedRotate(vec2f(0.5f,0.5f), vec2f(x,y), 1);
      // Restore any preFPV zoom level, then clear it.
      arcballCamera->setZoomLevel(preFPVZoom + arcballCamera->getZoomLevel());
      preFPVZoom = 0.f;
      arcballCamera->setCenter(vec3f(res.worldPosition));
    }
    c["lookAt"] = vec3f(res.worldPosition);
    updateCamera();
  }
}

void MainWindow::keyboardMotion()
{
  if (!(g_camMoveX || g_camMoveY || g_camMoveZ || g_camMoveE || g_camMoveA
          || g_camMoveR))
    return;

  auto sensitivity = maxMoveSpeed;
  if (glfwGetKey(glfwWindow, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
    sensitivity *= fineControl;

  // 6 degrees of freedom, four arrow keys? no problem.
  double inOut = g_camMoveZ;
  double leftRight = g_camMoveX;
  double upDown = g_camMoveY;
  double roll = g_camMoveR;
  double elevation = g_camMoveE;
  double azimuth = g_camMoveA;

  if (inOut) {
    arcballCamera->dolly(inOut * sensitivity);
  }
  if (leftRight) {
    arcballCamera->pan(vec2f(leftRight, 0) * sensitivity);
  }
  if (upDown) {
    arcballCamera->pan(vec2f(0, upDown) * sensitivity);
  }
  if (elevation) {
    arcballCamera->constrainedRotate(
        vec2f(-.5, 0), vec2f(-.5, elevation * .005 * sensitivity), 0);
  }
  if (azimuth) {
    arcballCamera->constrainedRotate(
        vec2f(0, -.5), vec2f(azimuth * .005 * sensitivity, -0.5), 1);
  }
  if (roll) {
    arcballCamera->constrainedRotate(
        vec2f(-.5, 0), vec2f(-.5, roll * .005 * sensitivity), 2);
  }
  updateCamera();
}

void MainWindow::motion(const vec2f &position)
{
  if (frame->pauseRendering)
    return;

  const vec2f mouse = position * contentScale;
  if (previousMouse != vec2f(-1)) {
    const bool leftDown =
        glfwGetMouseButton(glfwWindow, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    const bool rightDown =
        glfwGetMouseButton(glfwWindow, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
    const bool middleDown =
        glfwGetMouseButton(glfwWindow, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS;
    const vec2f prev = previousMouse;

    bool cameraChanged = leftDown || rightDown || middleDown;

    auto sensitivity = maxMoveSpeed;
    if (glfwGetKey(glfwWindow, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
      sensitivity *= fineControl;

    auto displaySize = windowSize * contentScale;

    const vec2f mouseFrom(clamp(prev.x * 2.f / displaySize.x - 1.f, -1.f, 1.f),
        clamp(prev.y * 2.f / displaySize.y - 1.f, -1.f, 1.f));
    const vec2f mouseTo(clamp(mouse.x * 2.f / displaySize.x - 1.f, -1.f, 1.f),
        clamp(mouse.y * 2.f / displaySize.y - 1.f, -1.f, 1.f));

    if (leftDown) {
      arcballCamera->constrainedRotate(mouseFrom,
          lerp(sensitivity, mouseFrom, mouseTo),
          g_rotationConstraint);
    } else if (rightDown) {
      if (glfwGetKey(glfwWindow, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        arcballCamera->dolly((mouseTo - mouseFrom).y * sensitivity);
      else
        arcballCamera->zoom((mouseTo - mouseFrom).y * sensitivity);
    } else if (middleDown) {
      arcballCamera->pan((mouseTo - mouseFrom) * sensitivity);
    }

    if (cameraChanged)
      updateCamera();
  }

  previousMouse = mouse;
}

void MainWindow::mouseButton(const vec2f &position)
{
  if (frame->pauseRendering)
    return;

  if (glfwGetKey(glfwWindow, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS
      && glfwGetMouseButton(glfwWindow, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
    vec2f scaledPosition = position * contentScale;
    pickCenterOfRotation(scaledPosition.x, scaledPosition.y);
  }
}

void MainWindow::mouseWheel(const vec2f &scroll)
{
  if (!scroll || frame->pauseRendering)
    return;

  // scroll is +/- 1 for horizontal/vertical mouse-wheel motion

  auto sensitivity = maxMoveSpeed;
  if (glfwGetKey(glfwWindow, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
    sensitivity *= fineControl;

  if (scroll.y) {
    auto &camera = frame->child("camera");
    if (camera.hasChild("fovy")) {
      auto fovy = camera["fovy"].valueAs<float>();
      fovy = std::min(180.f, std::max(0.f, fovy + scroll.y * sensitivity));
      camera["fovy"] = fovy;
      updateCamera();
    }
  }

  // XXX anything interesting to do with horizontal scroll wheel?
  // Perhaps cycle through renderer types?  Or toggle denoiser or navigation?
}

void MainWindow::display()
{
  static auto displayStart = std::chrono::high_resolution_clock::now();

  if (optAutorotate) {
    vec2f from(0.f, 0.f);
    vec2f to(autorotateSpeed * 0.001f, 0.f);
    arcballCamera->rotate(from, to);
    updateCamera();
  }

  if (g_animatingPath) {
    static int framesPaused = 0;
    CameraState current = g_camPath[g_camCurrentPathIndex];
    arcballCamera->setState(current);
    updateCamera();

    // pause at the end of the path
    if (g_camCurrentPathIndex == (int) g_camPath.size() - 1) {
      framesPaused++;
      int framesToWait = g_camPathPause * ImGui::GetIO().Framerate;
      if (framesPaused > framesToWait) {
        framesPaused = 0;
        g_camCurrentPathIndex = 0;
      }
    } else {
      g_camCurrentPathIndex++;
    }
  }

  keyboardMotion();

  if (displayCallback)
    displayCallback(this);

  updateTitleBar();

  auto &frameBuffer = frame->childAs<sg::FrameBuffer>("framebuffer");
  fbSize = frameBuffer.child("size").valueAs<vec2i>();

  if (frame->frameIsReady()) {
    if (!frame->isCanceled()) {
      // display frame rate in window title
      auto displayEnd = std::chrono::high_resolution_clock::now();
      auto durationMilliseconds =
          std::chrono::duration_cast<std::chrono::milliseconds>(
              displayEnd - displayStart);

      latestFPS = 1000.f / float(durationMilliseconds.count());

      // map OSPRay framebuffer, update OpenGL texture with contents, then unmap
      waitOnOSPRayFrame();

      // Only enabled if they exist
      optShowAlbedo &= frameBuffer.hasAlbedoChannel();
      optShowDepth &= frameBuffer.hasDepthChannel();

      auto *mappedFB = (void *)frame->mapFrame(optShowDepth
              ? OSP_FB_DEPTH
              : (optShowAlbedo ? OSP_FB_ALBEDO : OSP_FB_COLOR));

      // This needs to query the actual framebuffer format
      const GLenum glType =
          frameBuffer.isFloatFormat() ? GL_FLOAT : GL_UNSIGNED_BYTE;

      // Only create the copy if it's needed
      float *pDepthCopy = nullptr;
      if (optShowDepth) {
        // Create a local copy and don't modify OSPRay buffer
        const auto *mappedDepth = static_cast<const float *>(mappedFB);
        std::vector<float> depthCopy(
            mappedDepth, mappedDepth + fbSize.x * fbSize.y);
        pDepthCopy = depthCopy.data();

        // Scale OSPRay's 0 -> inf depth range to OpenGL 0 -> 1, ignoring all
        // inf values
        float minDepth = rkcommon::math::inf;
        float maxDepth = rkcommon::math::neg_inf;
        for (auto depth : depthCopy) {
          if (isinf(depth))
            continue;
          minDepth = std::min(minDepth, depth);
          maxDepth = std::max(maxDepth, depth);
        }

        const float rcpDepthRange = 1.f / (maxDepth - minDepth);

        // Inverted depth (1.0 -> 0.0) may be more meaningful
        if (optShowDepthInvert)
          std::transform(depthCopy.begin(),
              depthCopy.end(),
              depthCopy.begin(),
              [&](float depth) {
                return (1.f - (depth - minDepth) * rcpDepthRange);
              });
        else
          std::transform(depthCopy.begin(),
              depthCopy.end(),
              depthCopy.begin(),
              [&](float depth) { return (depth - minDepth) * rcpDepthRange; });
      }

      glBindTexture(GL_TEXTURE_2D, framebufferTexture);
      glTexImage2D(GL_TEXTURE_2D,
          0,
          optShowAlbedo ? gl_rgb_format : gl_rgba_format,
          fbSize.x,
          fbSize.y,
          0,
          optShowDepth ? GL_LUMINANCE : (optShowAlbedo ? GL_RGB : GL_RGBA),
          glType,
          optShowDepth ? pDepthCopy : mappedFB);

      frame->unmapFrame(mappedFB);

      // save frame to a file, if requested
      if (g_saveNextFrame) {
        saveCurrentFrame();
        g_saveNextFrame = false;
      }
    }

    // Start new frame and reset frame timing interval start
    displayStart = std::chrono::high_resolution_clock::now();
    startNewOSPRayFrame();
  }

  // Allow OpenGL to show linear buffers as sRGB.
  if (uiDisplays_sRGB && !frameBuffer.isSRGB())
    glEnable(GL_FRAMEBUFFER_SRGB);

  // clear current OpenGL color buffer
  glClear(GL_COLOR_BUFFER_BIT);

  // render textured quad with OSPRay frame buffer contents
  vec2f border(0.f);
  if (lockAspectRatio) {
    // when rendered aspect ratio doesn't match window, compute texture
    // coordinates to center the display
    float aspectCorrection = lockAspectRatio * static_cast<float>(windowSize.y)
        / static_cast<float>(windowSize.x);
    if (aspectCorrection > 1.f) {
      border.y = 1.f - aspectCorrection;
    } else {
      border.x = 1.f - 1.f / aspectCorrection;
    }
  }
  border *= 0.5f;
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

  glBegin(GL_QUADS);

  glTexCoord2f(border.x, border.y);
  glVertex2f(0, 0);

  glTexCoord2f(border.x, 1.f - border.y);
  glVertex2f(0, windowSize.y);

  glTexCoord2f(1.f - border.x, 1.f - border.y);
  glVertex2f(windowSize.x, windowSize.y);

  glTexCoord2f(1.f - border.x, border.y);
  glVertex2f(windowSize.x, 0);

  glEnd();

  glDisable(GL_FRAMEBUFFER_SRGB);

  if (showUi) {
    // Notify ImGui of the colorspace for color picker widgets
    // (to match the colorspace of the framebuffer)
    if (uiDisplays_sRGB || frameBuffer.isSRGB())
      ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_IsSRGB;

    buildUI();
    ImGui::Render();
    ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());

    ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_IsSRGB;
  } else {
    ImGui::EndFrame();
  }

  // Update and Render additional Platform Windows
  if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
    GLFWwindow *backup_current_context = glfwGetCurrentContext();
    ImGui::UpdatePlatformWindows();
    ImGui::RenderPlatformWindowsDefault();
    glfwMakeContextCurrent(backup_current_context);
  }

  // swap buffers
  glfwSwapBuffers(glfwWindow);
}

void MainWindow::startNewOSPRayFrame()
{
  frame->startNewFrame();
}

void MainWindow::waitOnOSPRayFrame()
{
  frame->waitOnFrame();
}

void MainWindow::updateTitleBar()
{
  std::stringstream windowTitle;
  windowTitle << "OSPRay Studio: ";

  auto &fb = frame->childAs<sg::FrameBuffer>("framebuffer");
  auto &v = frame->childAs<sg::Renderer>("renderer")["varianceThreshold"];
  auto varianceThreshold = v.valueAs<float>();

  if (frame->pauseRendering) {
    windowTitle << "rendering paused";
  } else if (frame->accumLimitReached()) {
    windowTitle << "accumulation limit reached";
  } else if (fb.variance() < varianceThreshold) {
    windowTitle << "varianceThreshold reached";
  } else {
    windowTitle << std::setprecision(3) << latestFPS << " fps";
    if (latestFPS < 2.f) {
      float progress = frame->frameProgress();
      windowTitle << " | ";
      int barWidth = 20;
      std::string progBar;
      progBar.resize(barWidth + 2);
      auto start = progBar.begin() + 1;
      auto end = start + progress * barWidth;
      std::fill(start, end, '=');
      std::fill(end, progBar.end(), '_');
      *end = '>';
      progBar.front() = '[';
      progBar.back() = ']';
      windowTitle << progBar;
    }
  }

  // Set indicator in the title bar for frame modified
  windowTitle << (frame->isModified() ? "*" : "");

  glfwSetWindowTitle(glfwWindow, windowTitle.str().c_str());
}

GLFWwindow *MainWindow::getGLFWWindow()
{
  return glfwWindow;
}

void MainWindow::buildUI()
{
  // build main menu and options
  buildMainMenu();

  // build window UIs as needed
  buildWindows();

  if (uiCallback) {
    uiCallback();
  }

  for (auto &p : pluginPanels)
    if (p->isShown())
      p->buildUI(ImGui::GetCurrentContext());
}

void MainWindow::refreshRenderer()
{
  auto &r = frame->childAs<sg::Renderer>("renderer");
  if (optPF >= 0)
    r.createChild("pixelFilter", "int", optPF);

  r.child("pixelSamples").setValue(optSPP);
  r.child("varianceThreshold").setValue(optVariance);
  if (r.hasChild("maxContribution") && maxContribution < (float)math::inf)
    r["maxContribution"].setValue(maxContribution);

  // Re-add the backplate on renderer change
  if (backPlateTexture != "") {
    auto backplateTex =
      sg::createNodeAs<sg::Texture2D>("map_backplate", "texture_2d");
    if (backplateTex->load(backPlateTexture, false, false))
      r.add(backplateTex);
    else {
      backplateTex = nullptr;
      backPlateTexture = "";
    }
  }
}

void MainWindow::refreshScene(bool resetCam)
{
  if (frameAccumLimit)
    frame->accumLimit = frameAccumLimit;
  // Check that the frame contains a world, if not create one
  auto world = frame->hasChild("world") ? frame->childNodeAs<sg::Node>("world")
                                        : sg::createNode("world", "world");
  if (optSceneConfig == "dynamic")
    world->child("dynamicScene").setValue(true);
  else if (optSceneConfig == "compact")
    world->child("compactMode").setValue(true);
  else if (optSceneConfig == "robust")
    world->child("robustMode").setValue(true);

  world->createChild(
      "materialref", "reference_to_material", defaultMaterialIdx);

  if (!filesToImport.empty())
    importFiles(world);
  else if (scene != "") {
    auto &gen = world->createChildAs<sg::Generator>(
        scene + "_generator", "generator_" + scene);
    gen.setMaterialRegistry(baseMaterialRegistry);
    // The generators should reset the camera
    resetCam = true;
  }

  if (world->isModified()) {
    // Cancel any in-progress frame as world->render() will modify live device
    // parameters
    frame->cancelFrame();
    frame->waitOnFrame();
    world->render();
  }

  frame->add(world);

  if (resetCam && !sgScene) {
    const auto &worldBounds = frame->child("world").bounds();
    arcballCamera.reset(new ArcballCamera(worldBounds, windowSize));
    lastCamXfm = arcballCamera->getTransform();
  }
  updateCamera();
  auto &fb = frame->childAs<sg::FrameBuffer>("framebuffer");
  fb.resetAccumulation();
}

void MainWindow::addToCommandLine(std::shared_ptr<CLI::App> app) {
  app->add_flag(
    "--animate",
    optAnimate,
    "enable loading glTF animations"
  );
}

bool MainWindow::parseCommandLine()
{
  int ac = studioCommon.argc;
  const char **av = studioCommon.argv;

  std::shared_ptr<CLI::App> app = std::make_shared<CLI::App>("OSPRay Studio GUI");
  StudioContext::addToCommandLine(app);
  MainWindow::addToCommandLine(app);
  try {
    app->parse(ac, av);
  } catch (const CLI::ParseError &e) {
    exit(app->exit(e));
  }

  // XXX: changing windowSize here messes causes some display scaling issues
  // because it desyncs window and framebuffer size with any scaling
  if (optResolution.x != 0) {
    windowSize = optResolution;
    glfwSetWindowSize(glfwWindow, optResolution.x, optResolution.y);
    reshape(windowSize);
  }
  rendererTypeStr = optRendererTypeStr;

  if (!filesToImport.empty()) {
    std::cout << "Import files from cmd line" << std::endl;
    refreshScene(true);
  }

  return true;
}

// Importer for all known file types (geometry and models)
void MainWindow::importFiles(sg::NodePtr world)
{
  std::vector<sg::NodePtr> cameras;
  if (optAnimate)
    animationManager = std::shared_ptr<AnimationManager>(new AnimationManager);

  for (auto file : filesToImport) {
    try {
      rkcommon::FileName fileName(file);

      // XXX: handling loading a scene here for now
      if (fileName.ext() == "sg") {
        sg::importScene(shared_from_this(), fileName);
        sgScene = true;
      } else {
        std::cout << "Importing: " << file << std::endl;

        auto importer = sg::getImporter(world, file);
        if (importer) {
          if (volumeParams->children().size() > 0) {
            auto vp = importer->getVolumeParams();
            for (auto &c : volumeParams->children()) {
              vp->remove(c.first);
              vp->add(c.second);
            }
          }

          importer->pointSize = pointSize;
          importer->setFb(frame->childAs<sg::FrameBuffer>("framebuffer"));
          importer->setMaterialRegistry(baseMaterialRegistry);
          importer->setCameraList(cameras);
          importer->setLightsManager(lightsManager);
          importer->setArguments(studioCommon.argc, (char**)studioCommon.argv);
          if (animationManager)
            importer->setAnimationList(animationManager->getAnimations());
          if (optInstanceConfig == "dynamic")
            importer->setInstanceConfiguration(
                sg::InstanceConfiguration::DYNAMIC);
          else if (optInstanceConfig == "compact")
            importer->setInstanceConfiguration(
                sg::InstanceConfiguration::COMPACT);
          else if (optInstanceConfig == "robust")
            importer->setInstanceConfiguration(
                sg::InstanceConfiguration::ROBUST);

          importer->importScene();
        }
      }
    } catch (const std::exception &e) {
      std::cerr << "Failed to open file '" << file << "'!\n";
      std::cerr << "   " << e.what() << std::endl;
    } catch (...) {
      std::cerr << "Failed to open file '" << file << "'!\n";
    }
  }
  filesToImport.clear();

  if (animationManager) {
    animationManager->init();
    animationWidget = std::shared_ptr<AnimationWidget>(
        new AnimationWidget("Animation Controls", animationManager));
    registerImGuiCallback([&]() { animationWidget->addAnimationUI(); });
  }

  if (cameras.size() > 0) {
    auto mainCamera = frame->child("camera").nodeAs<sg::Camera>();
    g_sceneCameras[mainCamera->child("uniqueCameraName")
                       .valueAs<std::string>()] = mainCamera;

    // populate cameras in camera editor in View menu
    for (auto &c : cameras)
      g_sceneCameras[c->child("uniqueCameraName").valueAs<std::string>()] = c;
  }
}

void MainWindow::saveCurrentFrame()
{
  int filenum = 0;
  char filename[64];
  const char *ext = screenshotFiletype.c_str();

  do
    std::snprintf(filename, 64, "studio.%04d.%s", filenum++, ext);
  while (std::ifstream(filename).good());
  int screenshotFlags = screenshotLayers << 3
      | screenshotNormal << 2 | screenshotDepth << 1 | screenshotAlbedo;

  auto &fb = frame->childAs<sg::FrameBuffer>("framebuffer");
  auto fbFloatFormat = fb["floatFormat"].valueAs<bool>();
  if (screenshotFlags > 0 && !fbFloatFormat)
    std::cout
        << " *** Cannot save additional information without changing FB format to float ***"
        << std::endl;
  frame->saveFrame(std::string(filename), screenshotFlags);
}

void MainWindow::pushLookMark()
{
  cameraStack.push_back(arcballCamera->getState());
  vec3f from = arcballCamera->eyePos();
  vec3f up = arcballCamera->upDir();
  vec3f at = arcballCamera->lookDir() + from;
  fprintf(stderr,
      "-vp %f %f %f -vu %f %f %f -vi %f %f %f\n",
      from.x,
      from.y,
      from.z,
      up.x,
      up.y,
      up.z,
      at.x,
      at.y,
      at.z);
}

void MainWindow::popLookMark()
{
  if (cameraStack.empty())
    return;
  CameraState cs = cameraStack.back();
  cameraStack.pop_back();

  arcballCamera->setState(cs);
  updateCamera();
}

// Main menu //////////////////////////////////////////////////////////////////

void MainWindow::buildMainMenu()
{
  // build main menu bar and options
  ImGui::BeginMainMenuBar();
  buildMainMenuFile();
  buildMainMenuEdit();
  buildMainMenuView();
  buildMainMenuPlugins();
  ImGui::EndMainMenuBar();
}

void MainWindow::buildMainMenuFile()
{
  static bool showImportFileBrowser = false;

  if (ImGui::BeginMenu("File")) {
    if (ImGui::MenuItem("Import ...", nullptr)) {
      showImportFileBrowser = true;
      optAnimate = false;
    } else if (ImGui::MenuItem("Import and animate ...", nullptr)) {
      showImportFileBrowser = true;
      optAnimate = true;
    }
    if (ImGui::BeginMenu("Demo Scene")) {
      for (size_t i = 0; i < g_scenes.size(); ++i) {
        if (ImGui::MenuItem(g_scenes[i].c_str(), nullptr)) {
          scene = g_scenes[i];
          refreshScene(true);
        }
      }
      ImGui::EndMenu();
    }
    ImGui::Separator();
    if (ImGui::BeginMenu("Save")) {
      if (ImGui::MenuItem("Scene (entire)")) {
        std::ofstream dump("studio_scene.sg");
        JSON j = {{"world", frame->child("world")},
            {"camera", arcballCamera->getState()},
            {"lightsManager", *lightsManager},
            {"materialRegistry", *baseMaterialRegistry}};
        dump << j.dump();
      }

      ImGui::Separator();
      if (ImGui::MenuItem("Materials (only)")) {
        std::ofstream materials("studio_materials.sg");
        JSON j = {{"materialRegistry", *baseMaterialRegistry}};
        materials << j.dump();
      }
      if (ImGui::MenuItem("Lights (only)")) {
        std::ofstream lights("studio_lights.sg");
        JSON j = {{"lightsManager", *lightsManager}};
        lights << j.dump();
      }
      if (ImGui::MenuItem("Camera (only)")) {
        std::ofstream camera("studio_camera.sg");
        JSON j = {{"camera", arcballCamera->getState()}};
        camera << j.dump();
      }

      ImGui::Separator();

      if (ImGui::MenuItem("Screenshot", "Ctrl+S", nullptr))
        g_saveNextFrame = true;

      static const std::vector<std::string> screenshotFiletypes =
          sg::getExporterTypes();

      static int screenshotFiletypeChoice = std::distance(
          screenshotFiletypes.begin(),
          std::find(
              screenshotFiletypes.begin(), screenshotFiletypes.end(), "png"));

      ImGui::SetNextItemWidth(5.f * ImGui::GetFontSize());
      if (ImGui::Combo("##screenshot_filetype",
              (int *)&screenshotFiletypeChoice,
              stringVec_callback,
              (void *)screenshotFiletypes.data(),
              screenshotFiletypes.size())) {
        screenshotFiletype = screenshotFiletypes[screenshotFiletypeChoice];
      }
      sg::showTooltip("Image filetype for saving screenshots");

      if (screenshotFiletype == "exr") {
        // the following options should be available only when FB format is
        // float.
        auto &fb = frame->childAs<sg::FrameBuffer>("framebuffer");
        auto fbFloatFormat = fb["floatFormat"].valueAs<bool>();
        if (ImGui::Checkbox("FB float format ", &fbFloatFormat))
          fb["floatFormat"] = fbFloatFormat;
        if (fbFloatFormat) {
          ImGui::Checkbox("albedo##screenshotAlbedo", &screenshotAlbedo);
          ImGui::SameLine();
          ImGui::Checkbox("layers as separate files", &screenshotLayers);
          ImGui::Checkbox("depth##screenshotDepth", &screenshotDepth);
          ImGui::Checkbox("normal##screenshotNormal", &screenshotNormal);
        }
      }

      ImGui::EndMenu();
    }
    ImGui::EndMenu();
  }

  // Leave the fileBrowser open until files are selected
  if (showImportFileBrowser) {
    if (fileBrowser(filesToImport, "Select Import File(s) - ", true)) {
      showImportFileBrowser = false;
      // do not reset camera when loading a scene file
      bool resetCam = true;
      for (auto &fn : filesToImport)
        if (rkcommon::FileName(fn).ext() == "sg")
          resetCam = false;
      refreshScene(resetCam);
    }
  }
}

void MainWindow::buildMainMenuEdit()
{
  if (ImGui::BeginMenu("Edit")) {
    // Scene stuff /////////////////////////////////////////////////////

    if (ImGui::MenuItem("Lights...", "", nullptr))
      showLightEditor = true;
    if (ImGui::MenuItem("Transforms...", "", nullptr))
      showTransformEditor = true;
    if (ImGui::MenuItem("Materials...", "", nullptr))
      showMaterialEditor = true;
    if (ImGui::MenuItem("Transfer Functions...", "", nullptr))
      showTransferFunctionEditor = true;
    if (ImGui::MenuItem("Isosurfaces...", "", nullptr))
      showIsosurfaceEditor = true;
    ImGui::Separator();

    if (ImGui::MenuItem("Clear scene"))
      g_clearSceneConfirm = true;

    ImGui::EndMenu();
  }

  if (g_clearSceneConfirm) {
    g_clearSceneConfirm = false;
    ImGui::OpenPopup("Clear scene");
  }

  if (ImGui::BeginPopupModal("Clear scene")) {
    ImGui::Text("Are you sure you want to clear the scene?");
    ImGui::Text("This will delete all objects, materials and lights.");

    if (ImGui::Button("No!"))
      ImGui::CloseCurrentPopup();
    ImGui::SameLine(ImGui::GetWindowWidth()-(8*ImGui::GetFontSize()));

    if (ImGui::Button("Yes, clear it")) {
      // Cancel any in-progress frame
      frame->cancelFrame();
      frame->waitOnFrame();
      frame->remove("world");
      lightsManager->clear();
      if(animationWidget) {
        animationWidget.reset();
        registerImGuiCallback(nullptr);
      }

      // TODO: lights caching to avoid complete re-importing after clearing
      sg::clearAssets();

      // Recreate MaterialRegistry, clearing old registry and all materials
      // Then, add the new one to the frame and set the renderer type
      baseMaterialRegistry = sg::createNodeAs<sg::MaterialRegistry>(
          "baseMaterialRegistry", "materialRegistry");
      frame->add(baseMaterialRegistry);
      baseMaterialRegistry->updateRendererType();

      scene = "";
      refreshScene(true);
      ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
  }
}

void MainWindow::buildMainMenuView()
{
  static bool showFileBrowser = false;
  static bool guizmoOn = false;
  if (ImGui::BeginMenu("View")) {
    // Camera stuff ////////////////////////////////////////////////////

    if (ImGui::MenuItem("Camera...", "", nullptr))
      showCameraEditor = true;
    if (ImGui::MenuItem("Center camera", "", nullptr)) {
      arcballCamera.reset(
          new ArcballCamera(frame->child("world").bounds(), windowSize));
      updateCamera();
    }

    static bool lockUpDir = false;
    if (ImGui::Checkbox("Lock UpDir", &lockUpDir))
      arcballCamera->setLockUpDir(lockUpDir);

    if (lockUpDir) {
      ImGui::SameLine();
      static int dir = 1;
      static int _dir = -1;
      ImGui::RadioButton("X##setUpDir", &dir, 0);
      ImGui::SameLine();
      ImGui::RadioButton("Y##setUpDir", &dir, 1);
      ImGui::SameLine();
      ImGui::RadioButton("Z##setUpDir", &dir, 2);
      if (dir != _dir) {
        arcballCamera->setUpDir(vec3f(dir == 0, dir == 1, dir == 2));
        _dir = dir;
      }
    }

    if (ImGui::MenuItem("Keyframes...", "", nullptr))
      showKeyframes = true;
    if (ImGui::MenuItem("Snapshots...", "", nullptr))
      showSnapshots = true;

    ImGui::Text("Camera Movement Speed:");
    ImGui::SetNextItemWidth(5 * ImGui::GetFontSize());
    ImGui::SliderFloat("Speed##camMov", &maxMoveSpeed, 0.1f, 5.0f);
    ImGui::SetNextItemWidth(5 * ImGui::GetFontSize());
    ImGui::SliderFloat("FineControl##camMov", &fineControl, 0.1f, 1.0f, "%0.2fx");
    sg::showTooltip("hold <left-Ctrl> for more sensitive camera movement.");

    ImGui::Separator();

    // Renderer stuff //////////////////////////////////////////////////

    if (ImGui::MenuItem("Renderer..."))
      showRendererEditor = true;
    ImGui::Checkbox("Rendering stats", &showRenderingStats);
    ImGui::Checkbox("Pause rendering", &frame->pauseRendering);
    ImGui::SetNextItemWidth(5 * ImGui::GetFontSize());
    ImGui::DragInt(
        "Limit accumulation", &frame->accumLimit, 1, 0, INT_MAX, "%d frames");
    // Although varianceThreshold is found under the renderer, add it here to
    // make it easier to find, alongside accumulation limit
    ImGui::SetNextItemWidth(5 * ImGui::GetFontSize());
    frame->childAs<sg::Renderer>("renderer")["varianceThreshold"].
      traverse<sg::GenerateImGuiWidgets>(sg::TreeState::ROOTOPEN);
    ImGui::Checkbox("Auto rotate", &optAutorotate);
    if (optAutorotate) {
      ImGui::SameLine();
      ImGui::SetNextItemWidth(5 * ImGui::GetFontSize());
      ImGui::SliderInt(" speed", &autorotateSpeed, 1, 100);
    }
    ImGui::Separator();

    // Framebuffer and window stuff ////////////////////////////////////

    if (ImGui::MenuItem("Framebuffer..."))
      showFrameBufferEditor = true;
    if (ImGui::MenuItem("Set background texture..."))
      showFileBrowser = true;
    if (!backPlateTexture.str().empty()) {
      ImGui::TextColored(ImVec4(.5f, .5f, .5f, 1.f),
          "current: %s",
          backPlateTexture.base().c_str());
      if (ImGui::MenuItem("Clear background texture")) {
        frame->cancelFrame();
        frame->waitOnFrame();
        backPlateTexture = "";
        // Needs to be removed from the renderer node and its OSPRay params
        auto &renderer = frame->childAs<sg::Renderer>("renderer");
        renderer.remove("map_backplate");
        renderer.handle().removeParam("map_backplate");
      }
    }
    if (ImGui::BeginMenu("Quick window size")) {
      const std::vector<vec2i> options = {{480, 270},
          {960, 540},
          {1280, 720},
          {1920, 1080},
          {2560, 1440},
          {3840, 2160}};
      for (auto &sizeChoice : options) {
        char label[64];
        snprintf(label,
            sizeof(label),
            "%s%d x %d",
            windowSize == sizeChoice ? "*" : " ",
            sizeChoice.x,
            sizeChoice.y);
        if (ImGui::MenuItem(label)) {
          glfwSetWindowSize(glfwWindow, sizeChoice.x, sizeChoice.y);
          reshape(sizeChoice);
        }
      }
      ImGui::EndMenu();
    }
    ImGui::Checkbox("Display as sRGB", &uiDisplays_sRGB);
    sg::showTooltip("Display linear framebuffers as sRGB,\n"
                    "maintains consistent display across all formats.");
    // Allows the user to cancel long frame renders, such as too-many spp or
    // very large resolution.  Don't wait on the frame-cancel completion as
    // this locks up the UI.  Note: Next frame-start following frame
    // cancellation isn't immediate.
    if (ImGui::MenuItem("Cancel frame"))
      frame->cancelFrame();

    ImGui::Separator();

    // UI options //////////////////////////////////////////////////////
    ImGui::Text("ui options");

    ImGui::Checkbox("Show tooltips", &g_ShowTooltips);
    if (g_ShowTooltips) {
      ImGui::SameLine();
      ImGui::SetNextItemWidth(5 * ImGui::GetFontSize());
      ImGui::DragInt("delay", &g_TooltipDelay, 50, 0, 1000, "%d ms");
    }

#if 1 // XXX example new features that need to be integrated
    ImGui::Separator();

    ImGuiIO& io = ImGui::GetIO();
    ImGui::CheckboxFlags(
        "DockingEnable", &io.ConfigFlags, ImGuiConfigFlags_DockingEnable);
    sg::showTooltip("[experimental] Menu docking");
    ImGui::CheckboxFlags(
        "ViewportsEnable", &io.ConfigFlags, ImGuiConfigFlags_ViewportsEnable);
    sg::showTooltip("[experimental] Mind blowing multi-viewports support");

    ImGui::Checkbox("Guizmo", &guizmoOn);

    ImGui::Text("...right-click to open pie menu...");
    pieMenu();
#endif

    ImGui::EndMenu();
  }

#if 1 // Guizmo shows outsize menu window
  if (guizmoOn) {
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration
      | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings
      | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav
      | ImGuiWindowFlags_NoMove;
    ImGui::SetNextWindowBgAlpha(0.75f);

    // Bottom right corner
    ImVec2 window_pos(ImGui::GetIO().DisplaySize.x,
        ImGui::GetIO().DisplaySize.y);
    ImVec2 window_pos_pivot(1.f, 1.f);
    ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);

    if (ImGui::Begin("###guizmo", &guizmoOn, flags)) {
      guizmo();
      ImGui::End();
    }
  }
#endif

  // Leave the fileBrowser open until file is selected
  if (showFileBrowser) {
    FileList fileList = {};
    if (fileBrowser(fileList, "Select Background Texture")) {
      showFileBrowser = false;

      if (!fileList.empty()) {
        backPlateTexture = fileList[0];

        auto backplateTex =
            sg::createNodeAs<sg::Texture2D>("map_backplate", "texture_2d");
        if (backplateTex->load(backPlateTexture, false, false))
          frame->child("renderer").add(backplateTex);
        else {
          backplateTex = nullptr;
          backPlateTexture = "";
        }
      }
    }
  }
}

void MainWindow::buildMainMenuPlugins()
{
  if (!pluginPanels.empty() && ImGui::BeginMenu("Plugins")) {
    for (auto &p : pluginPanels)
      if (ImGui::MenuItem(p->name().c_str()))
        p->setShown(true);

    ImGui::EndMenu();
  }
}

// Option windows /////////////////////////////////////////////////////////////

void MainWindow::buildWindows()
{
  if (showRendererEditor)
    buildWindowRendererEditor();
  if (showFrameBufferEditor)
    buildWindowFrameBufferEditor();
  if (showKeyframes)
    buildWindowKeyframes();
  if (showSnapshots)
    buildWindowSnapshots();
  if (showCameraEditor)
    buildWindowCameraEditor();
  if (showLightEditor)
    buildWindowLightEditor();
  if (showMaterialEditor)
    buildWindowMaterialEditor();
  if (showTransferFunctionEditor)
    buildWindowTransferFunctionEditor();
  if (showIsosurfaceEditor)
    buildWindowIsosurfaceEditor();
  if (showTransformEditor)
    buildWindowTransformEditor();
  if (showRenderingStats)
    buildWindowRenderingStats();
}

void MainWindow::buildWindowRendererEditor()
{
  if (!ImGui::Begin(
          "Renderer editor", &showRendererEditor, g_imguiWindowFlags)) {
    ImGui::End();
    return;
  }

  int whichRenderer =
      find(g_renderers.begin(), g_renderers.end(), rendererTypeStr)
      - g_renderers.begin();

  static int whichDebuggerType = 0;
  ImGui::PushItemWidth(10.f * ImGui::GetFontSize());
  if (ImGui::Combo("renderer##whichRenderer",
          &whichRenderer,
          rendererUI_callback,
          nullptr,
          g_renderers.size())) {
    rendererTypeStr = g_renderers[whichRenderer];

    if (rendererType == OSPRayRendererType::DEBUGGER)
      whichDebuggerType = 0; // reset UI if switching away
                             // from debug renderer

    if (rendererTypeStr == "scivis")
      rendererType = OSPRayRendererType::SCIVIS;
    else if (rendererTypeStr == "pathtracer")
      rendererType = OSPRayRendererType::PATHTRACER;
    else if (rendererTypeStr == "ao")
      rendererType = OSPRayRendererType::AO;
    else if (rendererTypeStr == "debug")
      rendererType = OSPRayRendererType::DEBUGGER;
#ifdef USE_MPI
    if (rendererTypeStr == "mpiRaycast")
      rendererType = OSPRayRendererType::MPIRAYCAST;
#endif
    else
      rendererType = OSPRayRendererType::OTHER;

    // Change the renderer type, if the new renderer is different.
    auto currentType = frame->childAs<sg::Renderer>("renderer").subType();
    auto newType = "renderer_" + rendererTypeStr;

    if (currentType != newType) {
      frame->createChildAs<sg::Renderer>("renderer", newType);
      refreshRenderer();
    }
  }

  auto &renderer = frame->childAs<sg::Renderer>("renderer");

  if (rendererType == OSPRayRendererType::DEBUGGER) {
    if (ImGui::Combo("debug type##whichDebugType",
            &whichDebuggerType,
            debugTypeUI_callback,
            nullptr,
            g_debugRendererTypes.size())) {
      renderer["method"] = g_debugRendererTypes[whichDebuggerType];
    }
  }

  renderer.traverse<sg::GenerateImGuiWidgets>(sg::TreeState::ROOTOPEN);

  ImGui::End();
}

void MainWindow::buildWindowFrameBufferEditor()
{
  if (!ImGui::Begin(
          "Framebuffer editor", &showFrameBufferEditor, g_imguiWindowFlags)) {
    ImGui::End();
    return;
  }

  auto &fb = frame->childAs<sg::FrameBuffer>("framebuffer");
  fb.traverse<sg::GenerateImGuiWidgets>(sg::TreeState::ALLOPEN);

  ImGui::Separator();

  static int whichBuffer = 0;
  ImGui::Text("Display Buffer");
  ImGui::RadioButton("color##displayColor", &whichBuffer, 0);

  if (!fb.hasAlbedoChannel() && !fb.hasDepthChannel()) {
    ImGui::TextColored(
        ImVec4(.5f, .5f, .5f, 1.f), "Enable float format for more buffers");
  }

  if (fb.hasAlbedoChannel()) {
    ImGui::SameLine();
    ImGui::RadioButton("albedo##displayAlbedo", &whichBuffer, 1);
  }
  if (fb.hasDepthChannel()) {
    ImGui::SameLine();
    ImGui::RadioButton("depth##displayDepth", &whichBuffer, 2);
    ImGui::SameLine();
    ImGui::RadioButton("invert depth##displayDepthInv", &whichBuffer, 3);
  }

  switch (whichBuffer) {
  case 0:
    optShowColor = true;
    optShowAlbedo = optShowDepth = optShowDepthInvert = false;
    break;
  case 1:
    optShowAlbedo = true;
    optShowColor = optShowDepth = optShowDepthInvert = false;
    break;
  case 2:
    optShowDepth = true;
    optShowColor = optShowAlbedo = optShowDepthInvert = false;
    break;
  case 3:
    optShowDepth = true;
    optShowDepthInvert = true;
    optShowColor = optShowAlbedo = false;
    break;
  }

  ImGui::Separator();

  ImGui::Text("Post-processing");
  if (fb.isFloatFormat()) {
    ImGui::Checkbox("Tonemap", &frame->toneMapFB);
    ImGui::SameLine();
    ImGui::Checkbox("Tonemap nav", &frame->toneMapNavFB);

    if (studioCommon.denoiserAvailable) {
      ImGui::Checkbox("Denoise", &frame->denoiseFB);
      ImGui::SameLine();
      ImGui::Checkbox("Denoise nav", &frame->denoiseNavFB);
    }
    if (frame->denoiseFB || frame->denoiseNavFB) {
      ImGui::Checkbox("Denoise only PathTracer", &frame->denoiseOnlyPathTracer);
      ImGui::Checkbox("Denoise on final frame", &frame->denoiseFBFinalFrame);
      ImGui::SameLine();
      // Add accum here for convenience with final-frame denoising
      ImGui::SetNextItemWidth(5 * ImGui::GetFontSize());
      ImGui::DragInt(
          "Limit accumulation", &frame->accumLimit, 1, 0, INT_MAX, "%d frames");
    }
  } else {
    ImGui::TextColored(
        ImVec4(.5f, .5f, .5f, 1.f), "Enable float format for post-processing");
  }

  ImGui::Separator();

  ImGui::Text("Scaling");
  {
    static const float scaleValues[9] = {
        0.25f, 0.5f, 0.75f, 1.f, 1.25f, 1.5f, 2.f, 4.f, 8.f};

    auto size = frame->child("windowSize").valueAs<vec2i>();
    char _label[56];
    auto createLabel = [&_label, size](std::string uniqueId, float v) {
      const vec2i _sz = v * size;
      snprintf(_label,
          sizeof(_label),
          "%1.2fx (%d,%d)##%s",
          v,
          _sz.x,
          _sz.y,
          uniqueId.c_str());
      return _label;
    };

    auto selectNewScale = [&](std::string id, const float _scale) {
      auto scale = _scale;
      auto custom = true;
      for (auto v : scaleValues) {
        if (ImGui::Selectable(createLabel(id, v), v == scale))
          scale = v;
        custom &= (v != scale);
      }

      ImGui::Separator();
      char cLabel[64];
      snprintf(cLabel, sizeof(cLabel), "custom %s", createLabel(id, scale));
      if (ImGui::BeginMenu(cLabel)) {
        ImGui::SetNextItemWidth(5 * ImGui::GetFontSize());
        ImGui::InputFloat("x##fb_scaling", &scale);
        ImGui::EndMenu();
      }

      return scale;
    };

    auto scale = frame->child("scale").valueAs<float>();
    ImGui::SetNextItemWidth(12 * ImGui::GetFontSize());
    if (ImGui::BeginCombo("Scale resolution", createLabel("still", scale))) {
      auto newScale = selectNewScale("still", scale);
      if (scale != newScale)
        frame->child("scale") = newScale;
      ImGui::EndCombo();
    }

    scale = frame->child("scaleNav").valueAs<float>();
    ImGui::SetNextItemWidth(12 * ImGui::GetFontSize());
    if (ImGui::BeginCombo("Scale Nav resolution", createLabel("nav", scale))) {
      auto newScale = selectNewScale("nav", scale);
      if (scale != newScale)
        frame->child("scaleNav") = newScale;
      ImGui::EndCombo();
    }
  }

  ImGui::Separator();

  ImGui::Text("Aspect Ratio");
  const float origAspect = lockAspectRatio;
  if (lockAspectRatio != 0.f) {
    ImGui::SameLine();
    ImGui::Text("locked at %f", lockAspectRatio);
    if (ImGui::Button("Unlock")) {
      lockAspectRatio = 0.f;
    }
  } else {
    if (ImGui::Button("Lock")) {
      lockAspectRatio =
          static_cast<float>(windowSize.x) / static_cast<float>(windowSize.y);
    }
    sg::showTooltip("Lock to current aspect ratio");
  }

  ImGui::SetNextItemWidth(5 * ImGui::GetFontSize());
  ImGui::InputFloat("Set", &lockAspectRatio);
  sg::showTooltip("Lock to custom aspect ratio");
  lockAspectRatio = std::max(lockAspectRatio, 0.f);

  if (origAspect != lockAspectRatio)
    reshape(windowSize);

  ImGui::End();
}

void MainWindow::buildWindowKeyframes()
{
  if (!ImGui::Begin("Keyframe editor", &showKeyframes, g_imguiWindowFlags)) {
    ImGui::End();
    return;
  }

  ImGui::SetNextItemWidth(25 * ImGui::GetFontSize());

  if (ImGui::Button("add")) { // add current camera state after the selected one
    if (cameraStack.empty()) {
      cameraStack.push_back(arcballCamera->getState());
      g_camSelectedStackIndex = 0;
    } else {
      cameraStack.insert(cameraStack.begin() + g_camSelectedStackIndex + 1,
          arcballCamera->getState());
      g_camSelectedStackIndex++;
    }
  }

  sg::showTooltip("insert a new keyframe after the selected keyframe, based\n"
                  "on the current camera state");

  ImGui::SameLine();
  if (ImGui::Button("remove")) { // remove the selected camera state
    cameraStack.erase(cameraStack.begin() + g_camSelectedStackIndex);
    g_camSelectedStackIndex = std::max(0, g_camSelectedStackIndex - 1);
  }
  sg::showTooltip("remove the currently selected keyframe");

  if (cameraStack.size() >= 2) {
    ImGui::SameLine();
    if (ImGui::Button(g_animatingPath ? "stop" : "play")) {
      g_animatingPath = !g_animatingPath;
      g_camCurrentPathIndex = 0;
      if (g_animatingPath)
        g_camPath = buildPath(cameraStack, g_camPathSpeed * 0.01);
    }
    ImGui::SameLine();
    ImGui::SetNextItemWidth(10 * ImGui::GetFontSize());
    ImGui::SliderFloat("speed##path", &g_camPathSpeed, 0.f, 10.0);
    sg::showTooltip("Animation speed for computed path.\n"
                    "Slow speeds may cause jitter for small objects");

    static bool showCameraPath = false;
    if (ImGui::Checkbox("show camera path", &showCameraPath)) {
      if (!showCameraPath) {
        frame->child("world").remove("cameraPath_xfm");
        refreshScene(false);
      } else {
        auto pathXfm = sg::createNode("cameraPath_xfm", "transform");

        const auto &worldBounds = frame->child("world").bounds();
        float pathRad = 0.0075f * reduce_min(worldBounds.size());
        std::vector<CameraState> cameraPath =
            buildPath(cameraStack, g_camPathSpeed * 0.01f);
        std::vector<vec4f> pathVertices; // position and radius
        for (const auto &state : cameraPath)
          pathVertices.emplace_back(state.position(), pathRad);
        pathVertices.emplace_back(cameraStack.back().position(), pathRad);

        std::vector<uint32_t> indexes(pathVertices.size());
        std::iota(indexes.begin(), indexes.end(), 0);

        std::vector<vec4f> colors(pathVertices.size());
        std::fill(colors.begin(), colors.end(), vec4f(0.8f, 0.4f, 0.4f, 1.f));

        const std::vector<uint32_t> mID = {
            static_cast<uint32_t>(baseMaterialRegistry->baseMaterialOffSet())};
        auto mat = sg::createNode("pathGlass", "thinGlass");
        baseMaterialRegistry->add(mat);

        auto path = sg::createNode("cameraPath", "geometry_curves");
        path->createChildData("vertex.position_radius", pathVertices);
        path->createChildData("vertex.color", colors);
        path->createChildData("index", indexes);
        path->createChild("type", "uchar", (unsigned char)OSP_ROUND);
        path->createChild("basis", "uchar", (unsigned char)OSP_CATMULL_ROM);
        path->createChildData("material", mID);
        path->child("material").setSGOnly();


        std::vector<vec3f> capVertexes;
        std::vector<vec4f> capColors;
        for (int i = 0; i < cameraStack.size(); i++) {
          capVertexes.push_back(cameraStack[i].position());
          if (i == 0)
            capColors.push_back(vec4f(.047f, .482f, .863f, 1.f));
          else
            capColors.push_back(vec4f(vec3f(0.8f), 1.f));
        }

        auto caps = sg::createNode("cameraPathCaps", "geometry_spheres");
        caps->createChildData("sphere.position", capVertexes);
        caps->createChildData("color", capColors);
        caps->child("color").setSGOnly();
        caps->child("radius") = pathRad * 1.5f;
        caps->createChildData("material", mID);
        caps->child("material").setSGOnly();

        pathXfm->add(path);
        pathXfm->add(caps);

        frame->child("world").add(pathXfm);
      }
    }
  }

  if (ImGui::ListBoxHeader("##")) {
    for (int i = 0; i < cameraStack.size(); i++) {
      if (ImGui::Selectable(
              (std::to_string(i) + ": " + to_string(cameraStack[i])).c_str(),
              (g_camSelectedStackIndex == (int) i))) {
        g_camSelectedStackIndex = i;
        arcballCamera->setState(cameraStack[i]);
        updateCamera();
      }
    }
    ImGui::ListBoxFooter();
  }

  ImGui::End();
}

void MainWindow::setCameraSnapshot(size_t snapshot)
{
  if (snapshot < cameraStack.size()) {
    const CameraState &cs = cameraStack.at(snapshot);
    arcballCamera->setState(cs);
    updateCamera();
  }
}

void MainWindow::buildWindowSnapshots()
{
  if (!ImGui::Begin("Camera snap shots", &showSnapshots, g_imguiWindowFlags)) {
    ImGui::End();
    return;
  }
  ImGui::Text("+ key to add new snapshots");
  for (size_t s = 0; s < cameraStack.size(); s++) {
    if (ImGui::Button(std::to_string(s).c_str())) {
      setCameraSnapshot(s);
    }
  }
  if (cameraStack.size()) {
    if (ImGui::Button("save to cams.json")) {
      std::ofstream cams("cams.json");
      if (cams) {
        JSON j = cameraStack;
        cams << j;
      }
    }
  }
  ImGui::End();
}

void MainWindow::buildWindowLightEditor()
{
  if (!ImGui::Begin("Light editor", &showLightEditor, g_imguiWindowFlags)) {
    ImGui::End();
    return;
  }

  auto &lights = lightsManager->children();
  static int whichLight = -1;

  // Validate that selected light is still a valid light.  Clear scene will
  // change the lights list, elsewhere.
  if (whichLight >= lights.size())
    whichLight = -1;

  ImGui::Text("lights");
  if (ImGui::ListBoxHeader("", 3)) {
    int i = 0;
    for (auto &light : lights) {
      if (ImGui::Selectable(light.first.c_str(), (whichLight == i))) {
        whichLight = i;
      }
      i++;
    }
    ImGui::ListBoxFooter();

    if (whichLight != -1) {
      ImGui::Text("edit");
      lightsManager->child(lights.at_index(whichLight).first)
          .traverse<sg::GenerateImGuiWidgets>(sg::TreeState::ROOTOPEN);
    }
  }

  if (lights.size() > 1) {
    if (ImGui::Button("remove")) {
      if (whichLight != -1) {
        lightsManager->removeLight(lights.at_index(whichLight).first);
        whichLight = std::max(0, whichLight - 1);
      }
    }
  }

  ImGui::Separator();

  ImGui::Text("new light");

  static std::string lightType = "";
  if (ImGui::Combo("type##whichLightType",
          &whichLightType,
          lightTypeUI_callback,
          nullptr,
          g_lightTypes.size())) {
    if (whichLightType > -1 && whichLightType < (int) g_lightTypes.size())
      lightType = g_lightTypes[whichLightType];
  }

  static bool lightNameWarning = false;
  static bool lightTexWarning = false;

  static char lightName[64] = "";
  if (ImGui::InputText("name", lightName, sizeof(lightName)))
    lightNameWarning = !(*lightName) || lightsManager->lightExists(lightName);

  // HDRI lights need a texture
  static bool showHDRIFileBrowser = false;
  static rkcommon::FileName texFileName("");
  if (lightType == "hdri") {
    // This field won't be typed into.
    ImGui::InputTextWithHint(
        "texture", "select...", (char *)texFileName.base().c_str(), 0);
    if (ImGui::IsItemClicked())
      showHDRIFileBrowser = true;
  } else
    lightTexWarning = false;

  // Leave the fileBrowser open until file is selected
  if (showHDRIFileBrowser) {
    FileList fileList = {};
    if (fileBrowser(fileList, "Select HDRI Texture")) {
      showHDRIFileBrowser = false;

      lightTexWarning = fileList.empty();
      if (!fileList.empty()) {
        texFileName = fileList[0];
      }
    }
  }

  if ((!lightNameWarning && !lightTexWarning)) {
    if (ImGui::Button("add")) {
      if (lightsManager->addLight(lightName, lightType)) {
        if (lightType == "hdri") {
          auto &hdri = lightsManager->child(lightName);
          hdri["filename"] = texFileName.str();
        }
        // Select newly added light
        whichLight = lights.size() - 1;
      } else {
        lightNameWarning = true;
      }
    }
    if (lightsManager->hasDefaultLight()) {
      auto &rmDefaultLight = lightsManager->rmDefaultLight;
      ImGui::SameLine();
      ImGui::Checkbox("remove default", &rmDefaultLight);
    }
  }

  if (lightNameWarning)
    ImGui::TextColored(
        ImVec4(1.f, 0.f, 0.f, 1.f), "Light must have unique non-empty name");
  if (lightTexWarning)
    ImGui::TextColored(ImVec4(1.f, 0.f, 0.f, 1.f), "No texture provided");

  ImGui::End();
}

void MainWindow::buildWindowCameraEditor()
{
  if (!ImGui::Begin("Camera editor", &showCameraEditor)) {
    ImGui::End();
    return;
  }

  // Only present selector UI if more than one camera
  if (!g_sceneCameras.empty()
      && ImGui::Combo("sceneCameras##whichCamera",
          &whichCamera,
          cameraUI_callback,
          nullptr,
          g_sceneCameras.size())) {
    if (whichCamera > -1 && whichCamera < (int)g_sceneCameras.size()) {
      auto &newCamera = g_sceneCameras.at_index(whichCamera);
      g_selectedSceneCamera = newCamera.second;
      auto hasParents = g_selectedSceneCamera->parents().size();
      frame->remove("camera");
      frame->add(g_selectedSceneCamera);

      if (g_selectedSceneCamera->hasChild("aspect"))
        lockAspectRatio =
            g_selectedSceneCamera->child("aspect").valueAs<float>();
      reshape(windowSize); // resets aspect
      if (!hasParents)
        updateCamera();
    }
  }

  // Change camera type
  ImGui::Text("Type:");
  static const std::vector<std::string> cameraTypes = {
      "perspective", "orthographic", "panoramic"};
  auto currentType = frame->childAs<sg::Camera>("camera").subType();
  for (auto &type : cameraTypes) {
    auto newType = "camera_" + type;
    ImGui::SameLine();
    if (ImGui::RadioButton(type.c_str(), currentType == newType)) {
      // Create new camera of new type
      frame->createChildAs<sg::Camera>("camera", newType);
      reshape(windowSize); // resets aspect
      updateCamera(); // resets position, direction, etc
      break;
    }
  }

  // UI Widget
  auto &camera = frame->childAs<sg::Camera>("camera");
  bool updated = false;
  camera.traverse<sg::GenerateImGuiWidgets>(sg::TreeState::ROOTOPEN, updated);

  ImGui::End();
}

void MainWindow::buildWindowMaterialEditor()
{
  if (!ImGui::Begin("Material editor", &showMaterialEditor)) {
    ImGui::End();
    return;
  }

  static std::vector<sg::NodeType> types{sg::NodeType::MATERIAL};
  static SearchWidget searchWidget(types, types, sg::TreeState::ALLCLOSED);
  static AdvancedMaterialEditor advMaterialEditor;

  searchWidget.addSearchBarUI(*baseMaterialRegistry);
  searchWidget.addSearchResultsUI(*baseMaterialRegistry);
  auto selected = searchWidget.getSelected();
  if (selected) {
    selected->traverse<sg::GenerateImGuiWidgets>(sg::TreeState::ROOTOPEN);
    ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(245, 200, 66, 255));
    if (ImGui::TreeNodeEx(
            "Advanced options##materials", ImGuiTreeNodeFlags_None)) {
      ImGui::PopStyleColor();
      advMaterialEditor.buildUI(baseMaterialRegistry, selected);
      ImGui::TreePop();
    } else {
      ImGui::PopStyleColor();
    }
  }

  if (baseMaterialRegistry->isModified()) {
    frame->cancelFrame();
    frame->waitOnFrame();
  }

  ImGui::End();
}

void MainWindow::buildWindowTransferFunctionEditor()
{
  if (!ImGui::Begin("Transfer Function editor", &showTransferFunctionEditor)) {
    ImGui::End();
    return;
  }

  // Gather all transfer functions in the scene
  sg::CollectTransferFunctions visitor;
  frame->traverse(visitor);
  auto &transferFunctions = visitor.transferFunctions;

  if (transferFunctions.empty()) {
    ImGui::Text("== empty == ");

  } else {
    ImGui::Text("TransferFunctions");
    static int whichTFn = -1;
    static std::string selected = "";

    // Called by TransferFunctionWidget to update selected TFn
    auto transferFunctionUpdatedCallback =
        [&](const range1f &valueRange,
            const std::vector<vec4f> &colorsAndOpacities) {
          if (whichTFn != -1) {
            auto &tfn =
                *(transferFunctions[selected]->nodeAs<sg::TransferFunction>());
            auto &colors = tfn.colors;
            auto &opacities = tfn.opacities;

            colors.resize(colorsAndOpacities.size());
            opacities.resize(colorsAndOpacities.size());

            // Separate out colors
            std::transform(colorsAndOpacities.begin(),
                colorsAndOpacities.end(),
                colors.begin(),
                [](vec4f c4) { return vec3f(c4[0], c4[1], c4[2]); });

            // Separate out opacities
            std::transform(colorsAndOpacities.begin(),
                colorsAndOpacities.end(),
                opacities.begin(),
                [](vec4f c4) { return c4[3]; });

            tfn.createChildData("color", colors);
            tfn.createChildData("opacity", opacities);
            tfn["valueRange"] = valueRange.toVec2();
          }
        };

    static TransferFunctionWidget transferFunctionWidget(
        transferFunctionUpdatedCallback,
        range1f(0.f, 1.f),
        "TransferFunctionEditor");

    if (ImGui::ListBoxHeader("", transferFunctions.size())) {
      int i = 0;
      for (auto t : transferFunctions) {
        if (ImGui::Selectable(t.first.c_str(), (whichTFn == i))) {
          whichTFn = i;
          selected = t.first;

          auto &tfn = *(t.second->nodeAs<sg::TransferFunction>());
          const auto numSamples = tfn.colors.size();

          if (numSamples > 1) {
            auto vRange = tfn["valueRange"].valueAs<vec2f>();

            // Create a c4 from c3 + opacity
            std::vector<vec4f> c4;

            if (tfn.opacities.size() != numSamples)
              tfn.opacities.resize(numSamples, tfn.opacities.back());

            for (int n = 0; n < numSamples; n++) {
              c4.emplace_back(vec4f(tfn.colors.at(n), tfn.opacities.at(n)));
            }

            transferFunctionWidget.setValueRange(range1f(vRange[0], vRange[1]));
            transferFunctionWidget.setColorsAndOpacities(c4);
          }
        }
        i++;
      }
      ImGui::ListBoxFooter();

      ImGui::Separator();
      if (whichTFn != -1) {
        transferFunctionWidget.updateUI();
      }
    }
  }

  ImGui::End();
}

void MainWindow::buildWindowIsosurfaceEditor()
{
  if (!ImGui::Begin("Isosurface editor", &showIsosurfaceEditor)) {
    ImGui::End();
    return;
  }

  // Specialized node vector list box
  using vNodePtr = std::vector<sg::NodePtr>;
  auto ListBox = [](const char *label, int *selected, vNodePtr &nodes) {
    auto getter = [](void *vec, int index, const char **name) {
      auto nodes = static_cast<vNodePtr *>(vec);
      if (0 > index || index >= (int) nodes->size())
        return false;
      // Need longer lifetime than this lambda?
      static std::string copy = "";
      copy = nodes->at(index)->name();
      *name = copy.data();
      return true;
    };

    if (nodes.empty())
      return false;
    return ImGui::ListBox(
        label, selected, getter, static_cast<void *>(&nodes), nodes.size());
  };

  // Gather all volumes in the scene
  vNodePtr volumes = {};
  for (auto &node : frame->child("world").children())
    if (node.second->type() == sg::NodeType::GENERATOR
        || node.second->type() == sg::NodeType::IMPORTER
        || node.second->type() == sg::NodeType::VOLUME) {
      auto volume =
          findFirstNodeOfType(node.second, sg::NodeType::VOLUME);
      if (volume)
        volumes.push_back(volume);
    }

  ImGui::Text("Volumes");
  ImGui::Text("(select to create an isosurface)");
  if (volumes.empty()) {
    ImGui::Text("== empty == ");

  } else {
    static int currentVolume = 0;
    if (ListBox("##Volumes", &currentVolume, volumes)) {
      auto selected = volumes.at(currentVolume);

      auto &world = frame->childAs<sg::World>("world");

      auto count = 1;
      auto surfName = selected->name() + "_surf";
      while (world.hasChild(surfName + std::to_string(count) + "_xfm"))
        count++;
      surfName += std::to_string(count);

      auto isoXfm =
          sg::createNode(surfName + "_xfm", "transform", affine3f{one});

      auto valueRange = selected->child("valueRange").valueAs<range1f>();

      auto isoGeom = sg::createNode(surfName, "geometry_isosurfaces");
      isoGeom->createChild("valueRange", "range1f", valueRange);
      isoGeom->child("valueRange").setSGOnly();
      isoGeom->createChild("isovalue", "float", valueRange.center());
      isoGeom->child("isovalue").setMinMax(valueRange.lower, valueRange.upper);

      uint32_t materialID = baseMaterialRegistry->baseMaterialOffSet();
      const std::vector<uint32_t> mID = {materialID};
      auto mat = sg::createNode(surfName, "obj");

      // Give it some editable parameters
      mat->createChild("kd", "rgb", "diffuse color", vec3f(0.8f));
      mat->createChild("ks", "rgb", "specular color", vec3f(0.f));
      mat->createChild("ns", "float", "shininess [2-10e4]", 10.f);
      mat->createChild("d", "float", "opacity [0-1]", 1.f);
      mat->createChild("tf", "rgb", "transparency filter color", vec3f(0.f));
      mat->child("ns").setMinMax(2.f,10000.f);
      mat->child("d").setMinMax(0.f,1.f);

      baseMaterialRegistry->add(mat);
      isoGeom->createChildData("material", mID);
      isoGeom->child("material").setSGOnly();

      auto &handle = isoGeom->valueAs<cpp::Geometry>();
      handle.setParam("volume", selected->valueAs<cpp::Volume>());

      isoXfm->add(isoGeom);

      world.add(isoXfm);
    }
  }

  // Gather all isosurfaces in the scene
  vNodePtr surfaces = {};
  for (auto &node : frame->child("world").children())
    if (node.second->type() == sg::NodeType::GENERATOR
        || node.second->type() == sg::NodeType::IMPORTER
        || node.second->type() == sg::NodeType::TRANSFORM) {
      auto surface =
          findFirstNodeOfType(node.second, sg::NodeType::GEOMETRY);
      if (surface && (surface->subType() == "geometry_isosurfaces"))
        surfaces.push_back(surface);
    }

  ImGui::Separator();
  ImGui::Text("Isosurfaces");
  if (surfaces.empty()) {
    ImGui::Text("== empty == ");
  } else {
    for (auto &surface : surfaces) {
      surface->traverse<sg::GenerateImGuiWidgets>();
      if (surface->isModified())
        break;
    }
  }

  ImGui::End();
}

void MainWindow::buildWindowTransformEditor()
{
  if (!ImGui::Begin("Transform Editor", &showTransformEditor)) {
    ImGui::End();
    return;
  }

  typedef sg::NodeType NT;

  auto toggleSearch = [&](std::vector<sg::NodePtr> &results, bool visible) {
    for (auto result : results)
      if (result->hasChild("visible"))
        result->child("visible").setValue(visible);
  };
  auto showSearch = [&](std::vector<sg::NodePtr> &r) { toggleSearch(r, true); };
  auto hideSearch = [&](std::vector<sg::NodePtr> &r) { toggleSearch(r, false); };

  auto &warudo = frame->child("world");
  auto toggleDisplay = [&](bool visible) {
    warudo.traverse<sg::SetParamByNode>(NT::GEOMETRY, "visible", visible);
  };
  auto showDisplay = [&]() { toggleDisplay(true); };
  auto hideDisplay = [&]() { toggleDisplay(false); };

  static std::vector<NT> searchTypes{NT::TRANSFORM, NT::GEOMETRY, NT::VOLUME};
  static std::vector<NT> displayTypes{NT::GENERATOR, NT::IMPORTER, NT::TRANSFORM};
  static SearchWidget searchWidget(searchTypes, displayTypes);

  searchWidget.addSearchBarUI(warudo);
  searchWidget.addCustomAction("show all", showSearch, showDisplay);
  searchWidget.addCustomAction("hide all", hideSearch, hideDisplay, true);
  searchWidget.addSearchResultsUI(warudo);

  auto selected = searchWidget.getSelected();
  if (selected) {
    selected->traverse<sg::GenerateImGuiWidgets>(sg::TreeState::ROOTOPEN);
  }

  ImGui::End();
}

void MainWindow::buildWindowRenderingStats()
{
  ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration
      | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings
      | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav
      | ImGuiWindowFlags_NoMove;
  ImGui::SetNextWindowBgAlpha(0.75f);

  // Bottom left corner
  static const float FROM_EDGE = 10.f;
  ImVec2 window_pos(FROM_EDGE, ImGui::GetIO().DisplaySize.y - FROM_EDGE);
  ImVec2 window_pos_pivot(0.f, 1.f);
  ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);

  if (!ImGui::Begin("Rendering stats", &showRenderingStats, flags)) {
    ImGui::End();
    return;
  }

  auto &fb = frame->childAs<sg::FrameBuffer>("framebuffer");
  auto variance = fb.variance();
  auto &v = frame->childAs<sg::Renderer>("renderer")["varianceThreshold"];
  auto varianceThreshold = v.valueAs<float>();

  std::string mode = frame->child("navMode").valueAs<bool>() ? "Nav" : "";
  float scale = frame->child("scale" + mode).valueAs<float>();

  ImGui::Text("renderer: %s", rendererTypeStr.c_str());
  ImGui::Text("frame size: (%d,%d)", windowSize.x, windowSize.y);
  ImGui::SameLine();
  ImGui::Text("x%1.2f", scale);
  ImGui::Text("framerate: %-4.1f fps", latestFPS);
  ImGui::Text("ui framerate: %-4.1f fps", ImGui::GetIO().Framerate);

  if (varianceThreshold == 0) {
    ImGui::Text("variance    : %-5.2f    ", variance);
  } else {
    ImGui::Text("variance    :");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(8 * ImGui::GetFontSize());
    float progress = varianceThreshold / variance;
    char message[64];
    snprintf(
        message, sizeof(message), "%.2f/%.2f", variance, varianceThreshold);
    ImGui::ProgressBar(progress, ImVec2(0.f, 0.f), message);
  }

  if (frame->accumLimit == 0) {
    ImGui::Text("accumulation: %d", frame->currentAccum);
  } else {
    ImGui::Text("accumulation:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(8 * ImGui::GetFontSize());
    float progress = float(frame->currentAccum) / frame->accumLimit;
    char message[64];
    snprintf(message,
        sizeof(message),
        "%d/%d",
        frame->currentAccum,
        frame->accumLimit);
    ImGui::ProgressBar(progress, ImVec2(0.f, 0.f), message);
  }

  ImGui::End();
}
