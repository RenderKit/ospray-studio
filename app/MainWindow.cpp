// Copyright 2018-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "MainWindow.h"
// imgui
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl2.h"
// stb_image
#include "stb_image_write.h"
// std
#include <chrono>
#include <iostream>
#include <stdexcept>
// ospray_sg
#include "sg/camera/Camera.h"
#include "sg/exporter/Exporter.h"
#include "sg/fb/FrameBuffer.h"
#include "sg/generator/Generator.h"
#include "sg/importer/Importer.h"
#include "sg/renderer/Renderer.h"
#include "sg/scene/lights/Lights.h"
#include "sg/scene/World.h"
#include "sg/visitors/GenerateImGuiWidgets.h"
#include "sg/visitors/Search.h"
#include "sg/visitors/PrintNodes.h"
#include "sg/visitors/SetParamByNode.h"
#include "sg/visitors/RefLinkNodes.h"
// rkcommon
#include "rkcommon/math/rkmath.h"
#include "rkcommon/os/FileName.h"
#include "rkcommon/utility/SaveImage.h"
#include "rkcommon/utility/getEnvVar.h"
// tiny_file_dialogs
#include "sg/scene/volume/StructuredSpherical.h"
#include "sg/scene/volume/Structured.h"
#include "sg/scene/volume/Vdb.h"
#include "tinyfiledialogs.h"
// cerealization
#include <cereal/archives/json.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/memory.hpp>
#include <queue>
#include <fstream>

static std::vector<CameraState> g_cameraStack;

// GUI mode entry point
void start_GUI_mode(int argc, const char *argv[])
{
  std::cerr << "GUI mode\n";

  std::ifstream cams("cams.json");
  if (cams) {
    cereal::JSONInputArchive iarchive(cams);
    iarchive(g_cameraStack);
  }
  bool denoiser = ospLoadModule("denoiser") == OSP_NO_ERROR;

  auto window = make_unique<MainWindow>(vec2i(1024, 768), denoiser);
  window->parseCommandLine(argc, argv);
  window->mainLoop();
  window.reset();
}


static ImGuiWindowFlags g_imguiWindowFlags = ImGuiWindowFlags_AlwaysAutoResize;

static bool g_quitNextFrame  = false;
static bool g_saveNextFrame  = false;
static bool g_animatingPath  = false;

static const std::vector<std::string> g_scenes = {"empty",
                                                  "multilevel_hierarchy",
                                                  "torus",
                                                  "texture_volume_test",
                                                  "tutorial_scene",
                                                  "random_spheres",
                                                  "wavelet",
                                                  "import volume",
                                                  "import",
                                                  "unstructured_volume",
                                                  "clouds"};

static const std::vector<std::string> g_renderers = {
    "scivis", "pathtracer", "debug"};

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

std::vector<std::string> g_matTypes = {
    "obj", "alloy", "glass", "carPaint", "luminous", "metal", "thinGlass"};

std::vector<CameraState> g_camAnchors;  // user-defined anchor states
std::vector<CameraState> g_camPath;     // interpolated path through anchors
int g_camSelectedAnchorIndex = 0;
int g_camCurrentPathIndex    = 0;
int g_camPathSpeed = 5;  // defined in hundredths (e.g. 10 = 10 * 0.01 = 0.1)
const int g_camPathPause = 2;  // _seconds_ to pause for at end of path
int g_rotationConstraint = -1;

const double CAM_MOVERATE = 10.0; //TODO: the constant should be scene dependent or user changeable
double g_camMoveX = 0.0;
double g_camMoveY = 0.0;
double g_camMoveZ = 0.0;
double g_camMoveA = 0.0;
double g_camMoveE = 0.0;
double g_camMoveR = 0.0;

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

bool matTypeUI_callback(void *, int index, const char **out_text)
{
  *out_text = g_matTypes[index].c_str();
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

MainWindow::MainWindow(const vec2i &windowSize, bool denoiser)
    : scene(g_scenes[0]), denoiserAvailable(denoiser)
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
            case GLFW_KEY_B:
                PRINT(activeWindow->frame->bounds());
                break;
            case GLFW_KEY_V:
                activeWindow->frame->child("camera").traverse<sg::PrintNodes>();
                break;
            case GLFW_KEY_SPACE:
                g_animatingPath       = !g_animatingPath;
                g_camCurrentPathIndex = 0;
                if (g_animatingPath) {
                    g_camPath = buildPath(g_camAnchors, g_camPathSpeed * 0.01);
                }
                break;
            case GLFW_KEY_EQUAL:
                activeWindow->pushLookMark();
                break;
            case GLFW_KEY_MINUS:
                activeWindow->popLookMark();
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

  // OSPRay setup //

  frame = sg::createNodeAs<sg::Frame>("main_frame", "frame");

  baseMaterialRegistry = sg::createNodeAs<sg::MaterialRegistry>(
      "baseMaterialRegistry", "materialRegistry");

  for (auto mat : g_matTypes) {
    baseMaterialRegistry->addNewSGMaterial(mat);
  }

  refreshRenderer();
  refreshScene(true);

  // trigger window reshape events with current window size
  glfwGetFramebufferSize(glfwWindow, &this->windowSize.x, &this->windowSize.y);
  reshape(this->windowSize);
}

MainWindow::~MainWindow()
{
  ImGui_ImplOpenGL2_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
  glfwTerminate();
}

MainWindow *MainWindow::getActiveWindow()
{
  return activeWindow;
}

std::shared_ptr<sg::Frame> MainWindow::getFrame() {
  return frame;
}

void MainWindow::registerDisplayCallback(
    std::function<void(MainWindow *)> callback)
{
  displayCallback = callback;
}

void MainWindow::registerKeyCallback(
    std::function<
        void(MainWindow *, int key, int scancode, int action, int mods)>
        callback)
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
  frame->child("windowSize") = windowSize;
  frame->currentAccum = 0;

  // reset OpenGL viewport and orthographic projection
  glViewport(0, 0, windowSize.x, windowSize.y);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0.0, windowSize.x, 0.0, windowSize.y, -1.0, 1.0);

  // update camera
  arcballCamera->updateWindowSize(windowSize);

  auto &camera     = frame->child("camera");
  if (camera.hasChild("aspect"))
    camera["aspect"] = windowSize.x / float(windowSize.y);
}

void MainWindow::updateCamera()
{
  frame->currentAccum = 0;
  auto &camera = frame->child("camera");

  camera["position"]  = arcballCamera->eyePos();
  camera["direction"] = arcballCamera->lookDir();
  camera["up"]        = arcballCamera->upDir();
}

void MainWindow::pickCenterOfRotation(float x, float y)
{
  ospray::cpp::PickResult res;
  auto &fb = frame->childAs<sg::FrameBuffer>("framebuffer");
  auto &r = frame->childAs<sg::Renderer>("renderer");
  auto &c = frame->childAs<sg::Camera>("camera");
  auto &w = frame->childAs<sg::World>("world");

  x = clamp(x/windowSize.x, 0.f, 1.f);
  y = 1.f-clamp(y/windowSize.y, 0.f, 1.f);
  res = fb.handle().pick(r, c, w, x, y);
  if (res.hasHit)
  {
    arcballCamera->setCenter(vec3f(res.worldPosition));
    if (cancelFrameOnInteraction) {
      frame->cancelFrame();
      waitOnOSPRayFrame();
    }
    updateCamera();
  }
}

void MainWindow::keyboardMotion()
{
  if (!(g_camMoveX || g_camMoveY || g_camMoveZ ||
        g_camMoveE || g_camMoveA || g_camMoveR))
    return;

  auto sensitivity = 1.f;
  auto fineControl = 5.f;
  if (glfwGetKey(glfwWindow, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
    sensitivity /= fineControl;

  //6 degrees of freedom, four arrow keys? no problem.
  double inOut = g_camMoveZ;
  double leftRight = g_camMoveX;
  double upDown = g_camMoveY;
  double roll = g_camMoveR;
  double elevation = g_camMoveE;
  double azimuth = g_camMoveA;

  if (inOut)
  {
    arcballCamera->zoom(inOut * sensitivity);
  }
  if (leftRight)
  {
    arcballCamera->pan(vec2f(leftRight, 0) * sensitivity);
  }
  if (upDown)
  {
    arcballCamera->pan(vec2f(0, upDown) * sensitivity);
  }
  if (elevation)
  {
    arcballCamera->constrainedRotate(vec2f(-.5,0), vec2f(-.5,elevation*.005*sensitivity), 0);
  }
  if (azimuth)
  {
    arcballCamera->constrainedRotate(vec2f(0,-.5), vec2f(azimuth*.005*sensitivity,-0.5), 1);
  }
  if (roll)
  {
    arcballCamera->constrainedRotate(vec2f(-.5,0), vec2f(-.5,roll*.005*sensitivity), 2);
  }
  if (cancelFrameOnInteraction) {
    frame->cancelFrame();
    waitOnOSPRayFrame();
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
      const vec2f mouseFrom(
          clamp(prev.x * 2.f / windowSize.x - 1.f, -1.f, 1.f),
          clamp(prev.y * 2.f / windowSize.y - 1.f, -1.f, 1.f));
      const vec2f mouseTo(clamp(mouse.x * 2.f / windowSize.x - 1.f, -1.f, 1.f),
                          clamp(mouse.y * 2.f / windowSize.y - 1.f, -1.f, 1.f));
      arcballCamera->constrainedRotate(mouseFrom, lerp(sensitivity, mouseFrom, mouseTo), g_rotationConstraint);
    } else if (rightDown) {
      arcballCamera->zoom((mouse.y - prev.y) * sensitivity);
    } else if (middleDown) {
      arcballCamera->pan(vec2f(mouse.x - prev.x, prev.y - mouse.y) *
                         sensitivity);
    }

    if (cameraChanged) {
      frame->child("navMode") = true;
      if (cancelFrameOnInteraction) {
        frame->cancelFrame();
        waitOnOSPRayFrame();
      }
      updateCamera();
    }
  }

  previousMouse = mouse;
}

void MainWindow::mouseButton(const vec2f &position)
{
  if (frame->pauseRendering)
    return;

  if (glfwGetMouseButton(glfwWindow, GLFW_MOUSE_BUTTON_LEFT) == GLFW_RELEASE
      && glfwGetMouseButton(glfwWindow, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_RELEASE
      && glfwGetMouseButton(glfwWindow, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_RELEASE)
  {
    frame->child("navMode") = false;
    frame->cancelFrame();
  }

  if (glfwGetKey(glfwWindow, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS &&
      glfwGetMouseButton(glfwWindow, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
    pickCenterOfRotation(position.x,position.y);
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
    CameraState current     = g_camPath[g_camCurrentPathIndex];
    arcballCamera->setState(current);
    updateCamera();

    // pause at the end of the path
    if (g_camCurrentPathIndex == g_camPath.size() - 1) {
      framesPaused++;
      int framesToWait = g_camPathPause * ImGui::GetIO().Framerate;
      if (framesPaused > framesToWait) {
        framesPaused          = 0;
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
    // display frame rate in window title
    auto displayEnd = std::chrono::high_resolution_clock::now();
    auto durationMilliseconds =
      std::chrono::duration_cast<std::chrono::milliseconds>(displayEnd -
          displayStart);

    latestFPS = 1000.f / float(durationMilliseconds.count());

    // map OSPRay framebuffer, update OpenGL texture with contents, then unmap
    waitOnOSPRayFrame();

    // Only enabled if they exist
    showAlbedo &= frameBuffer.hasAlbedoChannel();
    showDepth &= frameBuffer.hasDepthChannel();

    auto *mappedFB =
      (void *)frame->mapFrame(showDepth ? OSP_FB_DEPTH
          : (showAlbedo ? OSP_FB_ALBEDO : OSP_FB_COLOR));

    // This needs to query the actual framebuffer format
    const GLenum glType = frameBuffer.hasFloatFormat()
      ? GL_FLOAT : GL_UNSIGNED_BYTE;

    // Only create the copy if it's needed
    float *pDepthCopy = nullptr;
    if (showDepth) {
      // Create a local copy and don't modify OSPRay buffer
      const auto *mappedDepth = static_cast<const float *>(mappedFB);
      std::vector<float> depthCopy(mappedDepth,
          mappedDepth + fbSize.x * fbSize.y);
      pDepthCopy = depthCopy.data();

      // Scale OSPRay's 0 -> inf depth range to OpenGL 0 -> 1, ignoring all
      // inf values
      float minDepth = rkcommon::math::inf;
      float maxDepth = rkcommon::math::neg_inf;
      for (auto depth: depthCopy) {
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
        std::transform(
            depthCopy.begin(),
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
  glBegin(GL_QUADS);

  glTexCoord2f(0.f, 0.f);
  glVertex2f(0.f, 0.f);

  glTexCoord2f(0.f, 1.f);
  glVertex2f(0.f, windowSize.y);

  glTexCoord2f(1.f, 1.f);
  glVertex2f(windowSize.x, windowSize.y);

  glTexCoord2f(1.f, 0.f);
  glVertex2f(windowSize.x, 0.f);

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
      auto end   = start + progress * barWidth;
      std::fill(start, end, '=');
      std::fill(end, progBar.end(), '_');
      *end            = '>';
      progBar.front() = '[';
      progBar.back()  = ']';
      windowTitle << progBar;
    }
  }

  glfwSetWindowTitle(glfwWindow, windowTitle.str().c_str());
  
}

GLFWwindow* MainWindow::getGLFWWindow() {
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
}

void MainWindow::refreshRenderer()
{
  auto &r = frame->createChild("renderer", "renderer_" + rendererTypeStr);

  if (optPF >= 0)
    r.createChild("pixelFilter", "int", optPF);

  if (rendererTypeStr != "debug") {
    baseMaterialRegistry->updateMaterialList(rendererTypeStr);
    r.createChildData("material", baseMaterialRegistry->cppMaterialList);
  }
  if (rendererTypeStr == "scivis" ||
      rendererTypeStr == "pathtracer") {
    if (useImportedTex) {
      const char *file = tinyfd_openFileDialog(
          "Import a texture from a file", "", 0, nullptr, nullptr, 0);
      std::shared_ptr<sg::Texture2D> backplateTex =
          std::static_pointer_cast<sg::Texture2D>(
              sg::createNode("map_backplate", "texture_2d"));
      backplateTex->load(file, false, false);
      r.add(backplateTex);
    }
  }
}

void MainWindow::refreshScene(bool resetCam)
{
  auto world = sg::createNode("world", "world");

  world->createChild(
      "materialref", "reference_to_material", defaultMaterialIdx);

  if (scene == "import") {
    if (!importGeometry(world)) {
      return;
    }
  } else if (scene == "import volume") {
    if (!importVolume(world)) {
      return;
    }
  } else {
    if (scene != "empty") {
      auto &gen = world->createChildAs<sg::Generator>("generator",
                                                      "generator_" + scene);
      gen.generateData();
    }
  }

  world->render();

  frame->add(world);

  if (resetCam)
    arcballCamera.reset(
        new ArcballCamera(frame->child("world").bounds(), windowSize));
  updateCamera();
  auto &fb = frame->childAs<sg::FrameBuffer>("framebuffer");
  fb.resetAccumulation();
}

void MainWindow::parseCommandLine(int &ac, const char **&av)
{
  for (int i = 1; i < ac; i++) {
    const auto arg = std::string(av[i]);
    if (arg.rfind("-", 0) != 0) {
      filesToImport.push_back(arg);
    } else if (arg == "-pf" || arg == "--pixelfilter") {
      optPF = max(0, atoi(av[i + 1]));
      rkcommon::removeArgs(ac, av, i, 2);
      --i;
    } else if(arg == "-linkNodes" || arg == "-ln") {
      linkNodes = true;
    }
  }

  if (!filesToImport.empty()) {
    importFiles();
  }
}

void MainWindow::importFiles()
{
  std::cout << "import files from cmd line" << std::endl;
  auto world = sg::createNode("world", "world");

  for (auto file : filesToImport) {
    try {
      rkcommon::FileName fileName(file);
      std::string nodeName = fileName.base() + "_importer";

      std::cout << "Importing: " << file << std::endl;
      auto oldRegistrySize = baseMaterialRegistry->children().size();
      auto importer        = sg::getImporter(file);
      if (importer != "") {
        auto &imp   = world->createChildAs<sg::Importer>(nodeName, importer);
        imp["file"] = std::string(file);
        imp.importScene(baseMaterialRegistry);

        if (baseMaterialRegistry->matImportsList.size() != 0) {
          for (auto &newMat : baseMaterialRegistry->matImportsList)
            g_matTypes.insert(g_matTypes.begin(), newMat);
        }

        if (baseMaterialRegistry->matImportsList.size() != 0) {
          for (auto &newMat : baseMaterialRegistry->matImportsList)
            g_matTypes.insert(g_matTypes.begin(), newMat);
        }

        if (oldRegistrySize != baseMaterialRegistry->children().size()) {
          auto newMats =
            baseMaterialRegistry->children().size() - oldRegistrySize;
          std::cout << "Importer added " << newMats << " material(s)"
            << std::endl;
          refreshRenderer();
        }
      }
    } catch (...) {
      std::cerr << "Failed to open file '" << file << "'!\n";
    }
  }

  if (linkNodes) {
    world->traverse<sg::RefLinkNodes>();

    // TODO Important: remove empty importer nodes as well 
  }

  world->render();

  frame->add(world);

  arcballCamera.reset(
      new ArcballCamera(frame->child("world").bounds(), windowSize));
  updateCamera();
}

bool MainWindow::importGeometry(std::shared_ptr<sg::Node> &world)
{
  const char *file = tinyfd_openFileDialog(
      "Import a scene from a file", "", 0, nullptr, nullptr, 0);

  if (file) {
    auto oldRegistrySize = baseMaterialRegistry->children().size();
    auto importer        = sg::getImporter(file);
    if (importer != "") {
      std::string nodeName = std::string(file) + "_importer";
      auto &imp   = world->createChildAs<sg::Importer>(nodeName, importer);
      imp["file"] = std::string(file);
      imp.importScene(baseMaterialRegistry);

      if (baseMaterialRegistry->matImportsList.size() != 0) {
        for (auto &newMat : baseMaterialRegistry->matImportsList)
          g_matTypes.insert(g_matTypes.begin(), newMat);
      }

      if (oldRegistrySize != baseMaterialRegistry->children().size()) {
        auto newMats =
            baseMaterialRegistry->children().size() - oldRegistrySize;
        std::cout << "Importer added " << newMats << " material(s)"
                  << std::endl;
        refreshRenderer();
      }
    } else {
      std::cout << "No importer for selected file, nothing to import!\n";
      return false;
    }
  } else {
    std::cout << "No file selected, nothing to import!\n";
    return false;
  }

  return true;
}

bool MainWindow::importVolume(std::shared_ptr<sg::Node> &world)
{
  const char *file = tinyfd_openFileDialog(
      "Import a volume from a file", "", 0, nullptr, nullptr, 0);

  if (file) {
    const std::string fs(file);
    std::shared_ptr<sg::Volume> volumeImport;
    if (fs.length() > 4 && fs.substr(fs.length()-4) == ".vdb")
    {
#if USE_OPENVDB
      auto vol = std::static_pointer_cast<sg::VdbVolume>(
          sg::createNode("volume", "volume_vdb"));
      vol->load(file);
      volumeImport = vol;
#else
      std::cout << "OpenVDB not enabled in build.  Rebuild Studio, selecting "
                   "ENABLE_OPENVDB in cmake." << std::endl;
      return false;
#endif
    }
    else
    {
      auto vol =
          std::static_pointer_cast<sg::StructuredVolume>(
              sg::createNode("volume", "structuredRegular"));
      vol->load(file);
      volumeImport = vol;
    }

    auto &tf = world->createChild("transferFunction", "transfer_function_jet");
    tf.add(volumeImport);

  } else {
    std::cout << "No file selected, nothing to import!\n";
    return false;
  }

  return true;
}

void MainWindow::saveCurrentFrame()
{
  std::string filename("studio.");
  filename += screenshotFiletype;
  int screenshotFlags = screenshotLayers << 3 | screenshotNormal << 2 |
                        screenshotDepth << 1 | screenshotAlbedo;
  frame->saveFrame(filename, screenshotFlags);
}

void MainWindow::pushLookMark()
{
  g_cameraStack.push_back(arcballCamera->getState());
  vec3f from = arcballCamera->eyePos();
  vec3f up   = arcballCamera->upDir();
  vec3f at   = arcballCamera->lookDir() + from;
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
  if (g_cameraStack.empty())
    return;
  CameraState cs = g_cameraStack.back();
  g_cameraStack.pop_back();
  arcballCamera->setState(cs);
  if (cancelFrameOnInteraction) {
    frame->cancelFrame();
    waitOnOSPRayFrame();
  }
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
  ImGui::EndMainMenuBar();
}

void MainWindow::buildMainMenuFile()
{
  if (ImGui::BeginMenu("File")) {
    if (ImGui::MenuItem("Import geometry...", nullptr)) {
      scene = "import";
      refreshScene(true);
    }
    if (ImGui::MenuItem("Import volume...", nullptr)) {
      scene = "import volume";
      refreshScene(true);
    }
    if (ImGui::BeginMenu("Demo Scene")) {
      //g_scene[0]='empty' works fine but not sure it makes sense as an user visible option
      for (int i = 1; i < g_scenes.size(); ++i) {
        if (ImGui::MenuItem(g_scenes[i].c_str(), nullptr)) {
           scene = g_scenes[i];
           auto numImportedMats = baseMaterialRegistry->matImportsList.size();
           g_matTypes.erase(g_matTypes.begin(), g_matTypes.begin() + numImportedMats);
           baseMaterialRegistry->rmMatImports();
           refreshScene(true);
        }
      }
      ImGui::EndMenu();
    }
    ImGui::EndMenu();
  }
}

void MainWindow::buildMainMenuEdit()
{
  if (ImGui::BeginMenu("Edit")) {

    ImGui::Text("general");

    int whichRenderer = find(g_renderers.begin(), g_renderers.end(), rendererTypeStr) - g_renderers.begin();

    static int whichDebuggerType = 0;
    if (ImGui::Combo("renderer##whichRenderer",
                     &whichRenderer,
                     rendererUI_callback,
                     nullptr,
                     g_renderers.size())) {
      rendererTypeStr = g_renderers[whichRenderer];

      if (rendererType == OSPRayRendererType::DEBUGGER)
        whichDebuggerType = 0;  // reset UI if switching away
                                // from debug renderer

      if (rendererTypeStr == "scivis")
        rendererType = OSPRayRendererType::SCIVIS;
      else if (rendererTypeStr == "pathtracer")
        rendererType = OSPRayRendererType::PATHTRACER;
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
    if (renderer.isModified())
      frame->cancelFrame();

    ImGui::Separator();

    auto &fb = frame->childAs<sg::FrameBuffer>("framebuffer");

    ImGui::Text("Display buffer options");
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
      showColor  = true;
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

    if (denoiserAvailable) {
      ImGui::Separator();
      ImGui::Text("Denoiser Options:");
      if (fb["allowDenoising"].valueAs<bool>()) {
        ImGui::Checkbox("denoise", &frame->denoiseFB);
        ImGui::SameLine();
        ImGui::Checkbox("denoiseNav", &frame->denoiseNavFB);
      } else
        ImGui::Text("- Check that FrameBuffer allowDenoising is enabled");
    }


    ImGui::Separator();
    // Expose entire framebuffer options
    fb.traverse<sg::GenerateImGuiWidgets>();

    ImGui::Separator();
    ImGui::Text("frame scaling");
    frame->child("windowSize").traverse<sg::GenerateImGuiWidgets>();
    ImGui::Text("framebuffer");
    ImGui::SameLine();
    fb["size"].traverse<sg::GenerateImGuiWidgets>();

    // XXX combine these two!  they're nearly identical
    if (ImGui::BeginMenu("Scale Resolution")) {
      auto scale    = frame->child("scale").valueAs<float>();
      auto oldScale = scale;
      auto custom   = true;
      auto values   = {0.25f, 0.5f, 0.75f, 1.f, 1.25f, 1.5f, 2.f, 4.f, 8.f};
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

      if (oldScale != scale) {
        frame->cancelFrame();
        frame->child("scale") = scale;
      }

      ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Scale Nav Resolution")) {
      auto scale    = frame->child("scaleNav").valueAs<float>();
      auto oldScale = scale;
      auto custom   = true;
      auto values   = {0.25f, 0.5f, 0.75f, 1.f, 1.25f, 1.5f, 2.f, 4.f, 8.f};
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

      if (oldScale != scale) {
        frame->cancelFrame();
        frame->child("scaleNav") = scale;
      }

      ImGui::EndMenu();
    }

    ImGui::Separator();
    ImGui::Text("scene");

    if (ImGui::MenuItem("Background texture...", "", nullptr)) {
      const char *file = tinyfd_openFileDialog(
          "Import a texture from a file", "", 0, nullptr, nullptr, 0);
      importedFilename = std::string(file);
      std::shared_ptr<sg::Texture2D> backplateTex =
          std::static_pointer_cast<sg::Texture2D>(
              sg::createNode("map_backplate", "texture_2d"));
      backplateTex->load(file, false, false);
      frame->child("renderer").add(backplateTex);
      useImportedTex = true;
    }
    if (useImportedTex) {
      ImGui::SameLine();
      ImGui::Text("[%s]", importedFilename.c_str());
    }

    if (ImGui::MenuItem("Lights...", "", nullptr))
      showLightEditor = true;
    if (ImGui::MenuItem("Materials...", "", nullptr))
      showMaterialEditor = true;

    ImGui::Separator();
    ImGui::Text("interaction");
    ImGui::Checkbox("cancel frame on interaction", &cancelFrameOnInteraction);
    ImGui::Checkbox("auto rotate", &autorotate);
    if (autorotate) {
      ImGui::SliderInt("auto rotate speed", &autorotateSpeed, 1, 100);
    }

    ImGui::Separator();
    ImGui::Text("export");
    static const std::vector<std::string> screenshotFiletypes =
        sg::getExporterTypes();

    static int screenshotFiletypeChoice = std::distance(
        screenshotFiletypes.begin(),
        std::find(
            screenshotFiletypes.begin(), screenshotFiletypes.end(), "png"));

    if (ImGui::Combo("Screenshot filetype",
                     (int *)&screenshotFiletypeChoice,
                     stringVec_callback,
                     (void *)screenshotFiletypes.data(),
                     screenshotFiletypes.size())) {
      screenshotFiletype = screenshotFiletypes[screenshotFiletypeChoice];
    }

    if (screenshotFiletype == "exr") {
      ImGui::Text("additional layers");
      ImGui::Checkbox("albedo##screenshotAlbedo", &screenshotAlbedo);
      ImGui::SameLine();
      ImGui::Checkbox("layers as separate files", &screenshotLayers);
      ImGui::Checkbox("depth##screenshotDepth", &screenshotDepth);
      ImGui::Checkbox("normal##screenshotNormal", &screenshotNormal);
    }

    ImGui::EndMenu();
  }
}

void MainWindow::buildMainMenuView()
{
  if (ImGui::BeginMenu("View")) {
    ImGui::Checkbox("Pause rendering", &frame->pauseRendering);
    ImGui::DragInt(
        "Limit accumulation", &frame->accumLimit, 1, 0, INT_MAX, "%d frames");
    if (ImGui::MenuItem("Screenshot", "s", nullptr))
      g_saveNextFrame = true;
    if (ImGui::MenuItem("Keyframes...", "", nullptr))
      showKeyframes = true;
    if (ImGui::MenuItem("Snapshots...", "", nullptr))
      showSnapshots = true;
    if (ImGui::MenuItem("Geometry...", "", nullptr))
      showGeometryViewer = true;
    ImGui::Checkbox("Rendering stats...", &showRenderingStats);
    ImGui::EndMenu();
  }
}

// Option windows /////////////////////////////////////////////////////////////

void MainWindow::buildWindows()
{
  if (showKeyframes)
    buildWindowKeyframes();
  if (showLightEditor)
    buildWindowLightEditor();
  if (showMaterialEditor)
    buildWindowMaterialEditor();
  if (showGeometryViewer)
    buildWindowGeometryViewer();
  if (showSnapshots)
    buildWindowSnapshots();
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
  if (ImGui::ListBoxHeader("##")) {
    if (ImGui::Button(
            "+")) {  // add current camera state after the selected one
      if (g_camAnchors.empty()) {
        g_camAnchors.push_back(arcballCamera->getState());
        g_camSelectedAnchorIndex = 0;
      } else {
        g_camAnchors.insert(g_camAnchors.begin() + g_camSelectedAnchorIndex + 1,
                            arcballCamera->getState());
        g_camSelectedAnchorIndex++;
      }
    }
    ImGui::SameLine();
    if (ImGui::Button("-")) {  // remove the selected camera state
      g_camAnchors.erase(g_camAnchors.begin() + g_camSelectedAnchorIndex);
      g_camSelectedAnchorIndex = std::max(0, g_camSelectedAnchorIndex - 1);
    }
    if (g_camAnchors.size() >= 2) {
      ImGui::SameLine();
      if (ImGui::Button(g_animatingPath ? "stop" : "play")) {
        g_animatingPath       = !g_animatingPath;
        g_camCurrentPathIndex = 0;
        if (g_animatingPath) {
          g_camPath = buildPath(g_camAnchors, g_camPathSpeed * 0.01);
        }
      }
      ImGui::SameLine();
      ImGui::SetNextItemWidth(10 * ImGui::GetFontSize());
      ImGui::SliderInt("speed##path", &g_camPathSpeed, 1, 10);
      if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip(
            "Animation speed for computed path. \n"
            "Slow speeds may cause jitter for small objects");
      }
    }
    for (int i = 0; i < g_camAnchors.size(); i++) {
      if (ImGui::Selectable(
              (std::to_string(i) + ": " + to_string(g_camAnchors[i])).c_str(),
              (g_camSelectedAnchorIndex == i))) {
        g_camSelectedAnchorIndex = i;
        arcballCamera->setState(g_camAnchors[i]);
        updateCamera();
      }
    }
    ImGui::ListBoxFooter();
    ImGui::End();
  }
}

void MainWindow::buildWindowSnapshots()
{
  if (!ImGui::Begin("Camera snap shots", &showSnapshots, g_imguiWindowFlags)) {
    ImGui::End();
    return;
  }
  ImGui::Text("+ key to add new snapshots");
  for (int s=0; s < g_cameraStack.size(); s++)
  {
    if (ImGui::Button(std::to_string(s).c_str())) {
        CameraState cs = g_cameraStack.at(s);
        arcballCamera->setState(cs);
        if (cancelFrameOnInteraction) {
          frame->cancelFrame();
          waitOnOSPRayFrame();
        }
        updateCamera();
    }
  }
  if (g_cameraStack.size())
  {
    if (ImGui::Button("save to cams.json")) {
      std::ofstream cams("cams.json");
      if (cams)
      {
        cereal::JSONOutputArchive oarchive(cams);
        oarchive(g_cameraStack);
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
  
  ospray::sg::NodePtr lightMan;

  if (frame->hasChild("lights")) {
    lightMan = frame->childNodeAs<sg::Lights>("lights");
  } else {
    auto &world = frame->child("world");
    lightMan = world.childNodeAs<sg::Lights>("lights");
  }

  auto &lights   = lightMan->children();
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
      lightMan->child(selectedLight)
          .traverse<sg::GenerateImGuiWidgets>(sg::TreeState::ROOTOPEN);
    }
  }

  if (lights.size() > 1) {
    if (ImGui::Button("remove")) {
      if (whichLight != -1) {
        lightMan->nodeAs<sg::Lights>()->removeLight(selectedLight);
        whichLight    = std::max(0, whichLight - 1);
        selectedLight = (*(lights.begin() + whichLight)).first;
      }
    }
  }

  ImGui::Separator();

  ImGui::Text("new light");

  ImGui::Combo("type##whichLightType",
               &whichLightType,
               lightTypeUI_callback,
               nullptr,
               g_lightTypes.size());

  static bool lightNameWarning = false;
  static bool lightTexWarning = false;
  static char lightName[64] = "";
  if (ImGui::InputText("name", lightName, 64))
    lightNameWarning = false;

  if (ImGui::Button("add")) {
    lightTexWarning = false;
    if (whichLightType > -1 && whichLightType < g_lightTypes.size()) {
      std::string lightType = g_lightTypes[whichLightType];

      if (lightMan->nodeAs<sg::Lights>()->addLight(lightName, lightType)) {
        // actually load the texture if add was successful
        if (lightType == "hdri") {
          auto &hdri = lightMan->child(lightName);
          const char *file = tinyfd_openFileDialog(
              "Import an HDRI texture from a file", "", 0, nullptr, nullptr, 0);

          if (file != NULL) {
            auto &hdriTex = hdri.createChild("map", "texture_2d");
            std::shared_ptr<sg::Texture2D> ast2d =
                hdriTex.nodeAs<sg::Texture2D>();
            ast2d->load(file, false, false);
          } else {
            // the user probably hit cancel in the dialog, or bad file
            lightTexWarning = true;
            // auto lightsman  = *lightMan;
            lightMan->nodeAs<sg::Lights>()->removeLight(lightName);
          }
        }
      } else {
        lightNameWarning = true;
      }
    }
  }

  if (lightNameWarning)
    ImGui::TextColored(ImVec4(1.f, 0.f, 0.f, 1.f),
                       "Light must have unique non-empty name");
  if (lightTexWarning)
    ImGui::TextColored(ImVec4(1.f, 0.f, 0.f, 1.f), "No texture provided");

  ImGui::End();
}

void MainWindow::buildWindowMaterialEditor()
{
  if (!ImGui::Begin("Material editor", &showMaterialEditor, g_imguiWindowFlags)) {
    ImGui::End();
    return;
  }

  if (rendererType != OSPRayRendererType::PATHTRACER) {
    ImGui::Text("materials only apply to pathtracer");
    ImGui::End();
    return;
  }

  ImGui::Text("available materials:");

  bool changed = false;
  int whichMaterial = -1;

  if (!baseMaterialRegistry->sgMaterialList.size())
  {
    ImGui::Text("  no materials in the scene");
  } else {

    if (ImGui::ListBoxHeader("", 7)) {
      int i = 0;
      for (auto &aMat : baseMaterialRegistry->sgMaterialList) {
        std::string toShow;
        if (i == defaultMaterialIdx)
          toShow = std::string(" *") + aMat->osprayMaterialType().c_str();
        else
          toShow = std::string("  ") + aMat->osprayMaterialType().c_str();

        if (ImGui::Selectable(toShow.c_str(), (whichMaterial == i)) && whichMaterial != defaultMaterialIdx) {
          whichMaterial = i;
          changed = true;
        }
        i++;
      }
      ImGui::ListBoxFooter();
    }
  }

  ImGui::End();

  if (changed && whichMaterial != -1) {
    defaultMaterialIdx = whichMaterial;
    refreshScene(false);
  }
}

void MainWindow::buildWindowGeometryViewer()
{
  if (!ImGui::Begin(
          "Geometry viewer",
          &showGeometryViewer)) {
    ImGui::End();
    return;
  }

  auto &fb = frame->childAs<sg::FrameBuffer>("framebuffer");

  auto replaceWorld = [&]() {
    auto aWholeNewWorld = sg::createNode("world", "world");
    auto weAreTheWorld = frame->child("world").children();

    for (auto &child : weAreTheWorld) {
      child.second->removeAllParents();
      aWholeNewWorld->add(child.second);
    }
    aWholeNewWorld->render();

    frame->remove("world");
    frame->add(aWholeNewWorld);
    frame->currentAccum = 0;

    fb.resetAccumulation();
  };

  static char searchTerm[1024] = "";
  static bool searched         = false;
  static std::vector<sg::Node *> results;

  auto doClear  = [&]() {
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

  if (ImGui::InputTextWithHint("##findgeometryviewer",
                               "search...",
                               searchTerm,
                               1024,
                               ImGuiInputTextFlags_CharsNoBlank |
                                   ImGuiInputTextFlags_AutoSelectAll |
                                   ImGuiInputTextFlags_EnterReturnsTrue)) {
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
    replaceWorld();
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
    replaceWorld();
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
      result->traverse<sg::GenerateImGuiWidgets>(sg::TreeState::ALLCLOSED,
                                                 userUpdated);
    }
  } else {
    for (auto &node : frame->child("world").children()) {
      if (node.second->type() == sg::NodeType::GENERATOR ||
          node.second->type() == sg::NodeType::IMPORTER) {
        node.second->traverse<sg::GenerateImGuiWidgets>(sg::TreeState::ROOTOPEN,
                                                        userUpdated);
      }
    }
  }
  if (userUpdated)
    replaceWorld();
  ImGui::EndChild();

  ImGui::End();
}

void MainWindow::buildWindowRenderingStats()
{
  ImGuiWindowFlags flags =
      ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize |
      ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
      ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove;
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
  ImGui::Text("framerate: %-5.1f fps", latestFPS);
  ImGui::Text("variance : %-5.2f    ", variance);
  if (frame->accumLimit > 0) {
    ImGui::Text("accumulation:");
    ImGui::SameLine();
    float progress = float(frame->currentAccum) / frame->accumLimit;
    std::string progressStr = std::to_string(frame->currentAccum) + "/" +
                              std::to_string(frame->accumLimit);
    ImGui::ProgressBar(progress, ImVec2(0.f, 0.f), progressStr.c_str());
  }

  ImGui::End();
}
