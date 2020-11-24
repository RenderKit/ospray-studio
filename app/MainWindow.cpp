// Copyright 2018-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "MainWindow.h"
// imgui
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl2.h"
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
#include "sg/scene/lights/Lights.h"
#include "sg/visitors/Commit.h"
#include "sg/visitors/GenerateImGuiWidgets.h"
#include "sg/visitors/PrintNodes.h"
#include "sg/visitors/Search.h"
#include "sg/visitors/SetParamByNode.h"
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
#include "widgets/FileBrowserWidget.h"
#include "widgets/TransferFunctionWidget.h"

using namespace ospray_studio;

static ImGuiWindowFlags g_imguiWindowFlags = ImGuiWindowFlags_AlwaysAutoResize;

static bool g_quitNextFrame = false;
static bool g_saveNextFrame = false;
static bool g_animatingPath = false;

static const std::vector<std::string> g_scenes = {"tutorial_scene",
    "random_spheres",
    "wavelet",
    "torus_volume",
    "unstructured_volume",
    "multilevel_hierarchy"};

static const std::vector<std::string> g_renderers = {
    "scivis", "pathtracer", "ao", "debug"};

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
    : StudioContext(_common), windowSize(_common.defaultSize), scene("")
{
  if (activeWindow != nullptr) {
    throw std::runtime_error("Cannot create more than one MainWindow!");
  }

  activeWindow = this;

  glfwSetErrorCallback(error_callback);

  // initialize GLFW
  if (!glfwInit()) {
    throw std::runtime_error("Failed to initialize GLFW!");
  }

  glfwWindowHint(GLFW_SRGB_CAPABLE, GLFW_TRUE);

  // create GLFW window
  glfwWindow = glfwCreateWindow(
      windowSize.x, windowSize.y, "OSPRay Tutorial", nullptr, nullptr);

  if (!glfwWindow) {
    glfwTerminate();
    throw std::runtime_error("Failed to create GLFW window!");
  }

  glfwMakeContextCurrent(glfwWindow);

  // Turn on SRGB conversion for OSPRay frame
  glEnable(GL_FRAMEBUFFER_SRGB);

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

  glfwSetMouseButtonCallback(glfwWindow, [](GLFWwindow *win, int, int, int) {
    ImGuiIO &io = ImGui::GetIO();
    if (!activeWindow->showUi || !io.WantCaptureMouse) {
      double x, y;
      glfwGetCursorPos(win, &x, &y);
      activeWindow->mouseButton(vec2f{float(x), float(y)});
    }
  });

  glfwSetCursorPosCallback(glfwWindow, [](GLFWwindow *, double x, double y) {
    ImGuiIO &io = ImGui::GetIO();
    if (!activeWindow->showUi || !io.WantCaptureMouse) {
      activeWindow->motion(vec2f{float(x), float(y)});
    }
  });

  ImGui::CreateContext();

  ImGui_ImplGlfw_InitForOpenGL(glfwWindow, true);
  ImGui_ImplOpenGL2_Init();

  // set initial OpenGL state
  glEnable(GL_TEXTURE_2D);
  glDisable(GL_LIGHTING);

  // create OpenGL frame buffer texture
  glGenTextures(1, &framebufferTexture);
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, framebufferTexture);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  refreshRenderer();
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
  pluginManager.removeAllPlugins();
}

void MainWindow::start()
{
  std::cerr << "GUI mode\n";

  // load plugins //

  for (auto &p : studioCommon.pluginsToLoad)
    pluginManager.loadPlugin(p);

  // create panels //
  // doing this outside constructor to ensure shared_from_this()
  // can wrap a valid weak_ptr (in constructor, not guaranteed)

  auto newPluginPanels =
      pluginManager.getAllPanelsFromPlugins(shared_from_this());
  std::move(newPluginPanels.begin(),
      newPluginPanels.end(),
      std::back_inserter(pluginPanels));

  std::ifstream cams("cams.json");
  if (cams) {
    JSON j;
    cams >> j;
    cameraStack = j.get<std::vector<CameraState>>();
  }

  parseCommandLine();
  mainLoop();
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
}

void MainWindow::setCameraState(CameraState &cs)
{
  arcballCamera->setState(cs);
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
    arcballCamera->setCenter(vec3f(res.worldPosition));
    updateCamera();
  }
}

void MainWindow::keyboardMotion()
{
  if (!(g_camMoveX || g_camMoveY || g_camMoveZ || g_camMoveE || g_camMoveA
          || g_camMoveR))
    return;

  auto sensitivity = 1.f;
  auto fineControl = 5.f;
  if (glfwGetKey(glfwWindow, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
    sensitivity /= fineControl;

  // 6 degrees of freedom, four arrow keys? no problem.
  double inOut = g_camMoveZ;
  double leftRight = g_camMoveX;
  double upDown = g_camMoveY;
  double roll = g_camMoveR;
  double elevation = g_camMoveE;
  double azimuth = g_camMoveA;

  if (inOut) {
    arcballCamera->zoom(inOut * sensitivity);
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

  const vec2f mouse(position.x, position.y);
  if (previousMouse != vec2f(-1)) {
    const bool leftDown =
        glfwGetMouseButton(glfwWindow, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    const bool rightDown =
        glfwGetMouseButton(glfwWindow, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
    const bool middleDown =
        glfwGetMouseButton(glfwWindow, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS;
    const vec2f prev = previousMouse;

    bool cameraChanged = leftDown || rightDown || middleDown;

    // XXX TODO make sensitivity UI adjustable
    auto sensitivity = 1.f;
    auto fineControl = 5.f;
    if (glfwGetKey(glfwWindow, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
      sensitivity /= fineControl;

    if (leftDown) {
      const vec2f mouseFrom(clamp(prev.x * 2.f / windowSize.x - 1.f, -1.f, 1.f),
          clamp(prev.y * 2.f / windowSize.y - 1.f, -1.f, 1.f));
      const vec2f mouseTo(clamp(mouse.x * 2.f / windowSize.x - 1.f, -1.f, 1.f),
          clamp(mouse.y * 2.f / windowSize.y - 1.f, -1.f, 1.f));
      arcballCamera->constrainedRotate(mouseFrom,
          lerp(sensitivity, mouseFrom, mouseTo),
          g_rotationConstraint);
    } else if (rightDown) {
      arcballCamera->zoom((mouse.y - prev.y) * sensitivity);
    } else if (middleDown) {
      arcballCamera->pan(
          vec2f(mouse.x - prev.x, prev.y - mouse.y) * sensitivity);
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
    pickCenterOfRotation(position.x, position.y);
  }
}

void MainWindow::display()
{
  static auto displayStart = std::chrono::high_resolution_clock::now();

  if (autorotate) {
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

  if (showUi)
    buildUI();

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
      showAlbedo &= frameBuffer.hasAlbedoChannel();
      showDepth &= frameBuffer.hasDepthChannel();

      auto *mappedFB = (void *)frame->mapFrame(showDepth
              ? OSP_FB_DEPTH
              : (showAlbedo ? OSP_FB_ALBEDO : OSP_FB_COLOR));

      // This needs to query the actual framebuffer format
      const GLenum glType =
          frameBuffer.hasFloatFormat() ? GL_FLOAT : GL_UNSIGNED_BYTE;

      // Only create the copy if it's needed
      float *pDepthCopy = nullptr;
      if (showDepth) {
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
        if (showDepthInvert)
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
          showAlbedo ? gl_rgb_format : gl_rgba_format,
          fbSize.x,
          fbSize.y,
          0,
          showDepth ? GL_LUMINANCE : (showAlbedo ? GL_RGB : GL_RGBA),
          glType,
          showDepth ? pDepthCopy : mappedFB);

      frame->unmapFrame(mappedFB);
    }

    // Start new frame and reset frame timing interval start
    displayStart = std::chrono::high_resolution_clock::now();
    startNewOSPRayFrame();
  }

  if (g_saveNextFrame) {
    saveCurrentFrame();
    g_saveNextFrame = false;
  }

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

  if (showUi) {
    ImGui::Render();
    ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
  } else {
    ImGui::EndFrame();
  }

  // swap buffers
  glfwSwapBuffers(glfwWindow);
}

void MainWindow::startNewOSPRayFrame()
{
  // The baseMaterialRegistry and lightsManager don't hang off the frame, so
  // must be checked separately.  If modified, notify the frame by modifying a
  // child.
  if (baseMaterialRegistry->isModified()) {
    baseMaterialRegistry->commit();
    baseMaterialRegistry->updateMaterialList(rendererTypeStr);
    auto &r = frame->childAs<sg::Renderer>("renderer");
    r.createChildData("material", baseMaterialRegistry->cppMaterialList);
    frame->child("navMode") = true; // navMode is perfect for this
  }

  if (lightsManager->isModified() || lightsManager->isStubborn) {
    lightsManager->updateWorld(frame->childAs<sg::World>("world"));
    frame->child("navMode") = true; // navMode is perfect for this
  }

  // UI responsiveness can be increased by knowing if we're in the middle of
  // ImGui widget interaction.
  ImGuiIO &io = ImGui::GetIO();
  bool interacting = ImGui::IsAnyMouseDown()
      && (io.WantCaptureMouse || io.WantCaptureKeyboard);

  // If in the middle of UI interaction, back off on how frequently scene
  // updates are committed.
  // 10 is a good imperical number.  If necessary, make UI adjustable
  static auto skipCount = 0;
  if (interacting)
    skipCount = skipCount > 0 ? skipCount - 1 : 10;

  frame->startNewFrame(interacting && !!skipCount);
}

void MainWindow::waitOnOSPRayFrame()
{
  frame->waitOnFrame();
}

void MainWindow::updateTitleBar()
{
  std::stringstream windowTitle;
  windowTitle << "OSPRay Studio: ";

  if (frame->pauseRendering) {
    windowTitle << "rendering paused";
  } else if (frame->accumLimitReached()) {
    windowTitle << "accumulation limit reached";
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
  auto &r = frame->createChildAs<sg::Renderer>(
      "renderer", "renderer_" + rendererTypeStr);

  if (optPF >= 0)
    r.createChild("pixelFilter", "int", optPF);

  if (rendererTypeStr != "debug") {
    baseMaterialRegistry->updateMaterialList(rendererTypeStr);
    r.createChildData("material", baseMaterialRegistry->cppMaterialList);
  }
  if (rendererTypeStr == "scivis" || rendererTypeStr == "pathtracer") {
    if (backPlateTexture != "") {
      auto backplateTex =
          sg::createNodeAs<sg::Texture2D>("map_backplate", "texture_2d");
      backplateTex->load(backPlateTexture, false, false);
      r.add(backplateTex);
    }
  }
}

void MainWindow::refreshScene(bool resetCam)
{
  // Check that the frame contains a world, if not create one
  auto world = frame->hasChild("world") ? frame->childNodeAs<sg::Node>("world")
                                        : sg::createNode("world", "world");

  world->createChild(
      "materialref", "reference_to_material", defaultMaterialIdx);

  if (!filesToImport.empty()) {
    // Cancel any in-progress frame since we're changing the world.
    frame->cancelFrame();
    frame->waitOnFrame();
    importFiles(world);
  } else {
    if (scene != "") {
      // Cancel any in-progress frame since we're changing the world.
      frame->cancelFrame();
      frame->waitOnFrame();
      auto &gen = world->createChildAs<sg::Generator>(
          "generator", "generator_" + scene);
      gen.setMaterialRegistry(baseMaterialRegistry);
      gen.generateData();
    }
  }

  if (baseMaterialRegistry->isModified())
    refreshRenderer();

  world->render();

  frame->add(world);

  if (resetCam && !sgScene) {
    const auto &worldBounds = frame->child("world").bounds();
    arcballCamera.reset(new ArcballCamera(worldBounds, windowSize));
  }
  updateCamera();
  auto &fb = frame->childAs<sg::FrameBuffer>("framebuffer");
  fb.resetAccumulation();
}

bool MainWindow::parseCommandLine()
{
  int ac = studioCommon.argc;
  const char **av = studioCommon.argv;

  for (int i = 1; i < ac; i++) {
    const auto arg = std::string(av[i]);
    if (arg.rfind("-", 0) != 0) {
      filesToImport.push_back(arg);
    } else if (arg == "-h" || arg == "--help") {
      printHelp();
      return false;
    } else if (arg == "-pf" || arg == "--pixelfilter") {
      optPF = max(0, atoi(av[i + 1]));
      rkcommon::removeArgs(ac, av, i, 2);
      --i;
    } else if (arg == "--animate" || arg == "-a") {
      animate = true;
    } else if (arg == "--dimensions" || arg == "-d") {
      const std::string dimX(av[++i]);
      const std::string dimY(av[++i]);
      const std::string dimZ(av[++i]);
      useVolumeParams = true;
      vp.dimensions = vec3i(std::stoi(dimX), std::stoi(dimY), std::stoi(dimZ));
    } else if (arg == "--gridSpacing" || arg == "-g") {
      const std::string gridSpacingX(av[++i]);
      const std::string gridSpacingY(av[++i]);
      const std::string gridSpacingZ(av[++i]);
      useVolumeParams = true;
      vp.gridSpacing =
          vec3f(stof(gridSpacingX), stof(gridSpacingY), stof(gridSpacingZ));
    } else if (arg == "--gridOrigin" || arg == "-o") {
      const std::string gridOriginX(av[++i]);
      const std::string gridOriginY(av[++i]);
      const std::string gridOriginZ(av[++i]);
      useVolumeParams = true;
      vp.gridOrigin =
          vec3f(stof(gridOriginX), stof(gridOriginY), stof(gridOriginZ));
    } else if (arg == "--voxelType" || arg == "-v") {
      auto voxelTypeStr = std::string(av[++i]);
      auto it           = sg::volumeVoxelType.find(voxelTypeStr);
      if (it != sg::volumeVoxelType.end()) {
        vp.voxelType = it->second;
        useVolumeParams = true;
      } else {
        throw std::runtime_error("improper -voxelType format requested");
      }
    } else if (arg == "--2160p")
      glfwSetWindowSize(glfwWindow, 3840, 2160);
    else if (arg == "--1440p")
      glfwSetWindowSize(glfwWindow, 2560, 1440);
    else if (arg == "--1080p")
      glfwSetWindowSize(glfwWindow, 1920, 1080);
    else if (arg == "--720p")
      glfwSetWindowSize(glfwWindow, 1280, 720);
    else if (arg == "--540p")
      glfwSetWindowSize(glfwWindow, 960, 540);
    else if (arg == "--270p")
      glfwSetWindowSize(glfwWindow, 480, 270);
  }

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
  static std::vector<sg::Animation> animations; // XXX

  for (auto file : filesToImport) {
    try {
      rkcommon::FileName fileName(file);

      // ALOK: handling loading a scene here for now
      if (fileName.ext() == "sg") {
        sg::importScene(shared_from_this(), fileName);
        sgScene = true;
      } else {
        std::cout << "Importing: " << file << std::endl;

        auto importer = sg::getImporter(world, file);
        if (importer) {
          // Could be any type of importer.  Need to pass the MaterialRegistry,
          // importer will use what it needs.
          if(useVolumeParams)
            importer->setVolumeParams(&vp);
       
          importer->setMaterialRegistry(baseMaterialRegistry);
          importer->setCameraList(cameras);
          importer->setAnimationList(animations);
          importer->importScene();
        }
      }
    } catch (...) {
      std::cerr << "Failed to open file '" << file << "'!\n";
    }
  }
  filesToImport.clear();
  sg::clearImporter();

  if (cameras.size() > 0) {
    auto &mainCamera = frame->child("camera");
    auto defaultCamera =
        sg::createNode("default_camera", frame->child("camera").subType());
    for (auto &c : mainCamera.children()) {
      defaultCamera->createChild(
          c.first, c.second->subType(), c.second->value());
    }

    g_sceneCameras["default camera"] = defaultCamera;

    // populate cameras in camera editor in View menu
    for (auto &c : cameras)
      g_sceneCameras[c->name()] = c;
  }

  if (animate && animations.size()) {
    allAnimationWidgets.push_back(std::shared_ptr<AnimationWidget>(
        new AnimationWidget(getFrame(), animations, "Animation Control")));

    registerImGuiCallback([&]() {
      for (size_t i = 0; i < allAnimationWidgets.size(); ++i)
        allAnimationWidgets[i]->addAnimationUI();
    });
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
  int screenshotFlags = screenshotMetaData << 4 | screenshotLayers << 3
      | screenshotNormal << 2 | screenshotDepth << 1 | screenshotAlbedo;

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
    } else if (ImGui::MenuItem("Import and animate ...", nullptr)) {
      showImportFileBrowser = true;
      animate = true;
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
    if (ImGui::MenuItem("Clear Scene...", nullptr)) {
      // Cancel any in-progress frame since we're removing the world.
      frame->cancelFrame();
      frame->waitOnFrame();
      frame->remove("world");

      // Recreate MaterialRegistry, clearing old registry and all materials
      baseMaterialRegistry = sg::createNodeAs<sg::MaterialRegistry>(
          "baseMaterialRegistry", "materialRegistry");

      scene = "";
      refreshScene(true);
    }
    if (ImGui::BeginMenu("Save...")) {
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
      if (g_ShowTooltips && ImGui::IsItemHovered()
          && ImGui::GetCurrentContext()->HoveredIdTimer
              > g_TooltipDelay * 0.001)
        ImGui::SetTooltip("%s", "Image filetype for saving screenshots");

      if (screenshotFiletype == "exr") {
        ImGui::Text("additional layers");
        ImGui::Checkbox("albedo##screenshotAlbedo", &screenshotAlbedo);
        ImGui::SameLine();
        ImGui::Checkbox("layers as separate files", &screenshotLayers);
        ImGui::Checkbox("depth##screenshotDepth", &screenshotDepth);
        ImGui::Checkbox("normal##screenshotNormal", &screenshotNormal);
        // implemented only as layers within single file for now
        ImGui::Checkbox("metaData##screenshotMetaData", &screenshotMetaData);
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

  if(screenshotMetaData)
    frame->child("world").child("saveMetaData").setValue(true);
}

void MainWindow::buildMainMenuEdit()
{
  if (ImGui::BeginMenu("Edit")) {
    ImGui::Text("general");

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
      else
        rendererType = OSPRayRendererType::OTHER;

      refreshRenderer();
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

    ImGui::Separator();

    auto &fb = frame->childAs<sg::FrameBuffer>("framebuffer");

    ImGui::Text("display buffer");
    static int whichBuffer = 0;
    ImGui::RadioButton("color##displayColor", &whichBuffer, 0);

    if (!fb.hasAlbedoChannel() && !fb.hasDepthChannel()) {
      ImGui::Text("- No other channels available");
      ImGui::Text("- Check that FrameBuffer allowDenoising is enabled");
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
      showColor = true;
      showAlbedo = showDepth = showDepthInvert = false;
      break;
    case 1:
      showAlbedo = true;
      showColor = showDepth = showDepthInvert = false;
      break;
    case 2:
      showDepth = true;
      showColor = showAlbedo = showDepthInvert = false;
      break;
    case 3:
      showDepth = true;
      showDepthInvert = true;
      showColor = showAlbedo = false;
      break;
    }

    ImGui::Separator();
    ImGui::Checkbox("toneMap", &frame->toneMapFB);
    ImGui::SameLine();
    ImGui::Checkbox("toneMapNav", &frame->toneMapNavFB);

    if (studioCommon.denoiserAvailable) {
      if (fb["allowDenoising"].valueAs<bool>()) {
        ImGui::Checkbox("denoise", &frame->denoiseFB);
        ImGui::SameLine();
        ImGui::Checkbox("denoiseNav", &frame->denoiseNavFB);
      } else
        ImGui::Text("- Check that frameBuffer's allowDenoising is enabled");
    }

    ImGui::Separator();
    fb.traverse<sg::GenerateImGuiWidgets>();

    ImGui::Text("frame scaling");
    frame->child("windowSize").traverse<sg::GenerateImGuiWidgets>();
    ImGui::Text("framebuffer");
    ImGui::SameLine();
    fb["size"].traverse<sg::GenerateImGuiWidgets>();

    // XXX combine these two!  they're nearly identical
    if (ImGui::BeginMenu("Scale Resolution")) {
      auto scale = frame->child("scale").valueAs<float>();
      auto oldScale = scale;
      auto custom = true;
      auto values = {0.25f, 0.5f, 0.75f, 1.f, 1.25f, 1.5f, 2.f, 4.f, 8.f};
      for (auto v : values) {
        char label[64];
        vec2i newSize = v * windowSize;
        snprintf(label,
            sizeof(label),
            "%s%1.2fx (%d,%d)",
            v == scale ? "*" : " ",
            v,
            newSize[0],
            newSize[1]);
        if (v == 1.f)
          ImGui::Separator();
        if (ImGui::MenuItem(label))
          scale = v;
        if (v == 1.f)
          ImGui::Separator();

        custom &= (v != scale);
      }

      ImGui::Separator();
      vec2i newSize = scale * windowSize;
      char label[64];
      snprintf(label,
          sizeof(label),
          "%scustom (%d,%d)",
          custom ? "*" : " ",
          newSize[0],
          newSize[1]);
      if (ImGui::BeginMenu(label)) {
        ImGui::InputFloat("x##fb_scaling", &scale);
        ImGui::EndMenu();
      }

      if (oldScale != scale)
        frame->child("scale") = scale;

      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Scale Nav Resolution")) {
      auto scale = frame->child("scaleNav").valueAs<float>();
      auto oldScale = scale;
      auto custom = true;
      auto values = {0.25f, 0.5f, 0.75f, 1.f, 1.25f, 1.5f, 2.f, 4.f, 8.f};
      for (auto v : values) {
        char label[64];
        vec2i newSize = v * windowSize;
        snprintf(label,
            sizeof(label),
            "%s%1.2fx (%d,%d)",
            v == scale ? "*" : " ",
            v,
            newSize[0],
            newSize[1]);
        if (v == 1.f)
          ImGui::Separator();
        if (ImGui::MenuItem(label))
          scale = v;
        if (v == 1.f)
          ImGui::Separator();

        custom &= (v != scale);
      }

      ImGui::Separator();
      vec2i newSize = scale * windowSize;
      char label[64];
      snprintf(label,
          sizeof(label),
          "%scustom (%d,%d)",
          custom ? "*" : " ",
          newSize[0],
          newSize[1]);
      if (ImGui::BeginMenu(label)) {
        ImGui::InputFloat("x##fb_scaling", &scale);
        ImGui::EndMenu();
      }

      if (oldScale != scale)
        frame->child("scaleNav") = scale;

      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Aspect Control")) {
      const float origAspect = lockAspectRatio;
      if (ImGui::MenuItem("Lock")) {
        lockAspectRatio =
            static_cast<float>(windowSize.x) / static_cast<float>(windowSize.y);
      }
      if (ImGui::MenuItem("Unlock")) {
        lockAspectRatio = 0.f;
      }
      ImGui::InputFloat("Set", &lockAspectRatio);
      lockAspectRatio = std::max(lockAspectRatio, 0.f);
      if (origAspect != lockAspectRatio) {
        reshape(windowSize);
      }
      ImGui::EndMenu();
    }

    ImGui::Separator();
    ImGui::Text("scene");

    static bool showFileBrowser = false;
    ImGui::Text("Background texture...");
    ImGui::SameLine();
    // This field won't be typed into.
    ImGui::SetNextItemWidth(10 * ImGui::GetFontSize());
    ImGui::InputTextWithHint(
        "##BPTex", "select...", (char *)backPlateTexture.base().c_str(), 0);
    if (ImGui::IsItemClicked())
      showFileBrowser = true;

    // Leave the fileBrowser open until file is selected
    if (showFileBrowser) {
      FileList fileList = {};
      if (fileBrowser(fileList, "Select Background Texture")) {
        showFileBrowser = false;

        if (!fileList.empty()) {
          backPlateTexture = fileList[0];

          auto backplateTex =
              sg::createNodeAs<sg::Texture2D>("map_backplate", "texture_2d");
          backplateTex->load(backPlateTexture, false, false);
          frame->child("renderer").add(backplateTex);
        }
      }
    }
    ImGui::SameLine();
    if (ImGui::Button("clear")) {
      backPlateTexture = "";
      refreshRenderer();
    }

    ImGui::EndMenu();
  }
}

void MainWindow::buildMainMenuView()
{
  if (ImGui::BeginMenu("View")) {
    if (ImGui::MenuItem("Keyframes...", "", nullptr))
      showKeyframes = true;
    if (ImGui::MenuItem("Snapshots...", "", nullptr))
      showSnapshots = true;
    ImGui::Separator();
    if (ImGui::MenuItem("Lights...", "", nullptr))
      showLightEditor = true;
    if (ImGui::MenuItem("Camera...", "", nullptr))
      showCameraEditor = true;
    if (ImGui::MenuItem("Center camera", "", nullptr)) {
      arcballCamera.reset(
          new ArcballCamera(frame->child("world").bounds(), windowSize));
      updateCamera();
    }

    ImGui::Separator();
    if (ImGui::MenuItem("Transforms...", "", nullptr))
      showTransformEditor = true;
    if (ImGui::MenuItem("Materials...", "", nullptr))
      showMaterialEditor = true;
    if (ImGui::MenuItem("Transfer Functions...", "", nullptr))
      showTransferFunctionEditor = true;
    if (ImGui::MenuItem("Isosurfaces...", "", nullptr))
      showIsosurfaceEditor = true;

    ImGui::Separator();
    ImGui::Checkbox("Rendering stats...", &showRenderingStats);
    ImGui::SameLine();
    ImGui::Checkbox("Pause rendering", &frame->pauseRendering);

    ImGui::SetNextItemWidth(5 * ImGui::GetFontSize());
    ImGui::DragInt(
        "Limit accumulation", &frame->accumLimit, 1, 0, INT_MAX, "%d frames");

    ImGui::Checkbox("auto rotate", &autorotate);
    if (autorotate) {
      ImGui::SameLine();
      ImGui::SetNextItemWidth(5 * ImGui::GetFontSize());
      ImGui::SliderInt(" speed", &autorotateSpeed, 1, 100);
    }

    ImGui::Separator();
    ImGui::Checkbox("Show Tooltips...", &g_ShowTooltips);
    if (g_ShowTooltips) {
      ImGui::SameLine();
      ImGui::SetNextItemWidth(5 * ImGui::GetFontSize());
      ImGui::DragInt("delay", &g_TooltipDelay, 50, 0, 1000, "%d ms");
    }
    ImGui::EndMenu();
  }
}

void MainWindow::buildMainMenuPlugins()
{
  if (!pluginPanels.empty() && ImGui::BeginMenu("Plugins")) {
    for (auto &p : pluginPanels) {
      bool show = p->isShown();
      if (ImGui::Checkbox(p->name().c_str(), &show))
        p->toggleShown();
    }

    ImGui::EndMenu();
  }
}

// Option windows /////////////////////////////////////////////////////////////

void MainWindow::buildWindows()
{
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
  if (g_ShowTooltips && ImGui::IsItemHovered()
      && ImGui::GetCurrentContext()->HoveredIdTimer > g_TooltipDelay * 0.001) {
    ImGui::SetTooltip(
        "insert a new keyframe after the selected keyframe based "
        "on the current camera state");
  }

  ImGui::SameLine();
  if (ImGui::Button("remove")) { // remove the selected camera state
    cameraStack.erase(cameraStack.begin() + g_camSelectedStackIndex);
    g_camSelectedStackIndex = std::max(0, g_camSelectedStackIndex - 1);
  }
  if (g_ShowTooltips && ImGui::IsItemHovered()
      && ImGui::GetCurrentContext()->HoveredIdTimer > g_TooltipDelay * 0.001) {
    ImGui::SetTooltip("remove the currently selected keyframe");
  }

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
    if (g_ShowTooltips && ImGui::IsItemHovered()
        && ImGui::GetCurrentContext()->HoveredIdTimer
            > g_TooltipDelay * 0.001) {
      ImGui::SetTooltip(
          "Animation speed for computed path. \n"
          "Slow speeds may cause jitter for small objects");
    }

    static bool showCameraPath = false;
    if (ImGui::Checkbox("show camera path", &showCameraPath)) {
      if (!showCameraPath) {
        frame->cancelFrame();
        frame->waitOnFrame();
        frame->child("world").remove("cameraPath");
        frame->child("world").remove("cameraPathCaps");
        refreshScene(false);
      } else {
        auto path = sg::createNode("cameraPath", "geometry_curves");

        const auto &worldBounds = frame->child("world").bounds();
        float pathRad = 0.01f * worldBounds.size().sum() / 3.f;
        std::vector<CameraState> cameraPath =
            buildPath(cameraStack, g_camPathSpeed * 0.01f);
        std::vector<vec4f> vertexes; // position and radius
        for (const auto &state : cameraPath)
          vertexes.emplace_back(state.position(), pathRad);
        vertexes.emplace_back(cameraStack.back().position(), pathRad);

        std::vector<uint32_t> indexes(vertexes.size());
        std::iota(indexes.begin(), indexes.end(), 0);

        std::vector<vec4f> colors(vertexes);
        std::fill(colors.begin(), colors.end(), vec4f(vec3f(0.8f), 1.f));

        const std::vector<uint32_t> mID = {
            static_cast<uint32_t>(baseMaterialRegistry->children().size())};
        auto mat = sg::createNode("pathGlass", "thinGlass");
        baseMaterialRegistry->add(mat);

        path->remove("radius");
        path->createChildData("vertex.position_radius", vertexes);
        path->createChildData("vertex.color", colors);
        path->createChildData("index", indexes);
        path->createChild("type", "uchar", (unsigned char)OSP_ROUND);
        path->createChild("basis", "uchar", (unsigned char)OSP_LINEAR);
        path->createChildData("material", mID);
        path->child("material").setSGOnly();

        frame->child("world").add(path);

        auto caps = sg::createNode("cameraPathCaps", "geometry_spheres");

        unsigned long f = 1, b = vertexes.size() - 3;
        std::vector<vec3f> capVertexes;
        std::vector<vec4f> capColors;
        for (int i = 0; i < cameraStack.size(); i++) {
          capVertexes.push_back(cameraStack[i].position());
          if (i == 0)
            capColors.push_back(vec4f(.047f, .482f, .863f, 1.f));
          else if (i == cameraStack.size() - 1)
            capColors.push_back(vec4f(1.f, .761f, .039f, 1.f));
          else
            capColors.push_back(vec4f(vec3f(0.2f), 1.f));
        }

        caps->createChildData("sphere.position", capVertexes);
        caps->createChildData("color", capColors);
        caps->child("color").setSGOnly();
        caps->child("radius") = pathRad * 1.5f;
        caps->createChildData("material", mID);
        caps->child("material").setSGOnly();

        frame->child("world").add(caps);
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
  static std::string selectedLight;

  ImGui::Text("lights");
  if (ImGui::ListBoxHeader("", 3)) {
    int i = 0;
    for (auto &light : lights) {
      if (ImGui::Selectable(light.first.c_str(), (whichLight == i))) {
        whichLight = i;
        selectedLight = light.first;
      }
      i++;
    }
    ImGui::ListBoxFooter();

    if (whichLight != -1) {
      ImGui::Text("edit");
      lightsManager->child(selectedLight)
          .traverse<sg::GenerateImGuiWidgets>(sg::TreeState::ROOTOPEN);
    }
  }

  if (lights.size() > 1) {
    if (ImGui::Button("remove")) {
      if (whichLight != -1) {
        lightsManager->removeLight(selectedLight);
        whichLight = std::max(0, whichLight - 1);
        selectedLight = (*(lights.begin() + whichLight)).first;
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
  }

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

  if ((!lightNameWarning && !lightTexWarning) && ImGui::Button("add")) {
    if (lightsManager->addLight(lightName, lightType)) {
      if (lightType == "hdri") {
        auto &hdri = lightsManager->child(lightName);
        auto &hdriTex = hdri.createChild("map", "texture_2d");
        auto ast2d = hdriTex.nodeAs<sg::Texture2D>();
        ast2d->load(texFileName, false, false);
        // When using an HDRI, set background color to black.  It's otherwise
        // confusing.  The user can still adjust it afterward.
        auto &r = frame->childAs<sg::Renderer>("renderer");
        r["backgroundColor"] = vec4f(0.f);
      }
    } else {
      lightNameWarning = true;
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

  if (ImGui::Combo("sceneCameras##whichCamera",
          &whichCamera,
          cameraUI_callback,
          nullptr,
          g_sceneCameras.size())) {
    if (whichCamera > -1 && whichCamera < (int) g_sceneCameras.size()) {
      auto &currentCamera = g_sceneCameras.at_index(whichCamera);
      auto &cameraNode = currentCamera.second->children();
      auto &camera = frame->childAs<sg::Camera>("camera");
      for (auto &c : cameraNode) {
        camera.add(c.second);
      }
      updateCamera();
    }
  }

  auto &camera = frame->childAs<sg::Camera>("camera");
  camera.traverse<sg::GenerateImGuiWidgets>(sg::TreeState::ROOTOPEN);
  ImGui::End();
}

void MainWindow::buildWindowMaterialEditor()
{
  if (!ImGui::Begin("Material editor", &showMaterialEditor)) {
    ImGui::End();
    return;
  }

  baseMaterialRegistry->traverse<sg::GenerateImGuiWidgets>();

  ImGui::End();
}

void MainWindow::buildWindowTransferFunctionEditor()
{
  if (!ImGui::Begin("Transfer Function editor", &showTransferFunctionEditor)) {
    ImGui::End();
    return;
  }

  // Gather all transfer functions in the scene
  std::map<std::string, sg::NodePtr> transferFunctions = {};
  for (auto &node : frame->child("world").children())
    if (node.second->type() == sg::NodeType::GENERATOR
        || node.second->type() == sg::NodeType::IMPORTER
        || node.second->type() == sg::NodeType::VOLUME) {
      auto tfn =
          findFirstNodeOfType(node.second, sg::NodeType::TRANSFER_FUNCTION);
      // node.first is a unique name. tfn->name() is always "transferFunction"
      if (tfn)
        transferFunctions[node.first] = tfn;
    }

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

            tfn["valueRange"] = valueRange.toVec2();
            tfn.createChildData("color", colors);
            tfn.createChildData("opacity", opacities);
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

#if 0 // XXX Needs to be fixed.  This overwrites the default transferfunction
          auto &tfn = *(t.second->nodeAs<sg::TransferFunction>());
          const auto numSamples = tfn.colors.size();

          if (numSamples > 1) {
            auto vRange = tfn["valueRange"].valueAs<vec2f>();

            std::vector<vec4f> c4(numSamples);
            for (int n = 0; n < numSamples; n++)
              c4.emplace_back(vec4f(tfn.colors.at(n), tfn.opacities.at(n)));

            transferFunctionWidget.setValueRange(range1f(vRange[0], vRange[1]));
            transferFunctionWidget.setColorsAndOpacities(c4);
          }
#endif
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
  static auto ListBox = [](const char *label, int *selected, vNodePtr &nodes) {
    static auto getter = [](void *vec, int index, const char **name) {
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

      auto isoGeom = sg::createNode(surfName, "geometry_isosurfaces");
      isoGeom->createChild("isovalue", "float", 0.f);
      isoGeom->child("isovalue").setMinMax(-1.f, 1.f);

      uint32_t materialID = baseMaterialRegistry->children().size();
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
    for (auto &surface : surfaces)
      surface->traverse<sg::GenerateImGuiWidgets>();
  }

  ImGui::End();
}

void MainWindow::buildWindowTransformEditor()
{
  if (!ImGui::Begin("Transform Editor", &showTransformEditor)) {
    ImGui::End();
    return;
  }

  static char searchTerm[1024] = "";
  static bool searched = false;
  static std::vector<sg::Node *> results;

  auto doClear = [&]() {
    searched = false;
    results.clear();
    searchTerm[0] = '\0';
  };
  auto doSearch = [&]() {
    if (std::string(searchTerm).size() > 0) {
      searched = true;
      results.clear();
      frame->traverse<sg::Search>(
          std::string(searchTerm), sg::NodeType::GEOMETRY, results);
    } else {
      doClear();
    }
  };

  if (ImGui::InputTextWithHint("##findTransformEditor",
          "search...",
          searchTerm,
          1024,
          ImGuiInputTextFlags_CharsNoBlank | ImGuiInputTextFlags_AutoSelectAll
              | ImGuiInputTextFlags_EnterReturnsTrue)) {
    doSearch();
  }
  ImGui::SameLine();
  if (ImGui::Button("find")) {
    doSearch();
  }
  ImGui::SameLine();
  if (ImGui::Button("clear")) {
    doClear();
  }

  if (ImGui::Button("show all")) {
    if (searched) {
      for (auto result : results)
        result->child("visible").setValue(true);
    } else {
      frame->child("world").traverse<sg::SetParamByNode>(
          sg::NodeType::GEOMETRY, "visible", true);
    }
  }

  ImGui::SameLine();
  if (ImGui::Button("hide all")) {
    if (searched) {
      for (auto result : results)
        result->child("visible").setValue(false);
    } else {
      frame->child("world").traverse<sg::SetParamByNode>(
          sg::NodeType::GEOMETRY, "visible", false);
    }
  }

  if (searched) {
    ImGui::SameLine();
    ImGui::Text(
        "%lu %s", results.size(), (results.size() == 1 ? "result" : "results"));
  }

  ImGui::BeginChild(
      "geometry", ImVec2(0, 0), true, ImGuiWindowFlags_HorizontalScrollbar);
  bool userUpdated = false;
  if (searched) {
    for (auto result : results) {
      result->traverse<sg::GenerateImGuiWidgets>(
          sg::TreeState::ALLCLOSED, userUpdated);
      // Don't continue traversing
      if (userUpdated) {
        result->commit();
        break;
      }
    }
  } else {
    for (auto &node : frame->child("world").children()) {
      if (node.second->type() == sg::NodeType::GENERATOR
          || node.second->type() == sg::NodeType::IMPORTER
          || node.second->type() == sg::NodeType::TRANSFORM) {
        node.second->traverse<sg::GenerateImGuiWidgets>(
            sg::TreeState::ROOTOPEN, userUpdated);
        // Don't continue traversing
        if (userUpdated) {
          node.second->commit();
          break;
        }
      }
    }
  }
  ImGui::EndChild();

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

  ImGui::Text("renderer: %s", rendererTypeStr.c_str());
  ImGui::Text("framerate: %-7.1f fps", latestFPS);
  ImGui::Text("ui framerate: %-7.1f fps", ImGui::GetIO().Framerate);
  ImGui::Text("variance : %-5.2f    ", variance);
  if (frame->accumLimit > 0) {
    ImGui::Text("accumulation:");
    ImGui::SameLine();
    float progress = float(frame->currentAccum) / frame->accumLimit;
    std::string progressStr = std::to_string(frame->currentAccum) + "/"
        + std::to_string(frame->accumLimit);
    ImGui::ProgressBar(progress, ImVec2(0.f, 0.f), progressStr.c_str());
  }

  ImGui::End();
}

void MainWindow::printHelp()
{
  const char *help = R"help(ospStudio gui [options] [file1 [file2 ...]]

    OPTIONS
    -h, --help               this help message
    -pf N, --pixelfilter N   set default pixel filter:
                               0 = point
                               1 = box
                               2 = Gaussian
                               3 = Mitchell-Netravali
                               4 = Blackman-Harris
    -a, --animate            enable loading glTF animations
    --2160p, --1440p,        set window/frame resolution
    --1080p, --720p,
    --540p, --270p
)help";
  std::cerr << help;
}
