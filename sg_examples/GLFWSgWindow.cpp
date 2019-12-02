// ======================================================================== //
// Copyright 2018-2019 Intel Corporation                                    //
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

#include "GLFWSgWindow.h"
// imgui
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl2.h"
// std
#include <chrono>
#include <iostream>
#include <stdexcept>
// ospray_sg
#include "sg/generator/Generator.h"
#include "sg/visitors/PrintNodes.h"

static bool g_quitNextFrame = false;

static const std::vector<std::string> g_scenes = {
    "tutorial_scene", "random_spheres", "wavelet"};

static const std::vector<std::string> g_renderers = {"scivis",
                                                     "raycast",
                                                     "raycast_vertexColor",
                                                     "primID",
                                                     "geomID",
                                                     "instID",
                                                     "dPds",
                                                     "dPdt",
                                                     "Ng",
                                                     "Ns",
                                                     "backfacing_Ng",
                                                     "backfacing_Ns"};

bool sceneUI_callback(void *, int index, const char **out_text)
{
  *out_text = g_scenes[index].c_str();
  return true;
}

bool rendererUI_callback(void *, int index, const char **out_text)
{
  *out_text = g_renderers[index].c_str();
  return true;
}

// GLFWSgWindow definitions ///////////////////////////////////////////////

GLFWSgWindow *GLFWSgWindow::activeWindow = nullptr;

GLFWSgWindow::GLFWSgWindow(const vec2i &windowSize) : scene(g_scenes[0])
{
  if (activeWindow != nullptr) {
    throw std::runtime_error("Cannot create more than one GLFWSgWindow!");
  }

  activeWindow = this;

  // initialize GLFW
  if (!glfwInit()) {
    throw std::runtime_error("Failed to initialize GLFW!");
  }

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

  // create GLFW window
  glfwWindow = glfwCreateWindow(
      windowSize.x, windowSize.y, "OSPRay Tutorial", nullptr, nullptr);

  if (!glfwWindow) {
    glfwTerminate();
    throw std::runtime_error("Failed to create GLFW window!");
  }

  glfwMakeContextCurrent(glfwWindow);

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

  // set GLFW callbacks
  glfwSetFramebufferSizeCallback(
      glfwWindow, [](GLFWwindow *, int newWidth, int newHeight) {
        activeWindow->reshape(vec2i{newWidth, newHeight});
      });

  glfwSetCursorPosCallback(glfwWindow, [](GLFWwindow *, double x, double y) {
    ImGuiIO &io = ImGui::GetIO();
    if (!activeWindow->showUi || !io.WantCaptureMouse) {
      activeWindow->motion(vec2f{float(x), float(y)});
    }
  });

  glfwSetKeyCallback(glfwWindow,
                     [](GLFWwindow *, int key, int, int action, int) {
                       if (action == GLFW_PRESS) {
                         switch (key) {
                         case GLFW_KEY_G:
                           activeWindow->showUi = !(activeWindow->showUi);
                           break;
                         case GLFW_KEY_Q:
                           g_quitNextFrame = true;
                           break;
                         case GLFW_KEY_P:
                           activeWindow->frame->traverse<sg::PrintNodes>();
                           break;
                         case GLFW_KEY_B:
                           PRINT(activeWindow->frame->bounds());
                           break;
                         }
                       }
                     });

  // OSPRay setup //

  frame = sg::createNodeAs<sg::Frame>("main_frame", "frame");

  refreshScene(true);

  // trigger window reshape events with current window size
  glfwGetFramebufferSize(glfwWindow, &this->windowSize.x, &this->windowSize.y);
  reshape(this->windowSize);
}

GLFWSgWindow::~GLFWSgWindow()
{
  ImGui_ImplOpenGL2_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
  glfwTerminate();
}

GLFWSgWindow *GLFWSgWindow::getActiveWindow()
{
  return activeWindow;
}

void GLFWSgWindow::registerDisplayCallback(
    std::function<void(GLFWSgWindow *)> callback)
{
  displayCallback = callback;
}

void GLFWSgWindow::registerImGuiCallback(std::function<void()> callback)
{
  uiCallback = callback;
}

void GLFWSgWindow::mainLoop()
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

void GLFWSgWindow::reshape(const vec2i &newWindowSize)
{
  windowSize = newWindowSize;

  auto &fb   = frame->child("framebuffer");
  fb["size"] = windowSize;

  // reset OpenGL viewport and orthographic projection
  glViewport(0, 0, windowSize.x, windowSize.y);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0.0, windowSize.x, 0.0, windowSize.y, -1.0, 1.0);

  // update camera
  arcballCamera->updateWindowSize(windowSize);

  auto &camera     = frame->child("camera");
  camera["aspect"] = windowSize.x / float(windowSize.y);
}

void GLFWSgWindow::updateCamera()
{
  auto &camera = frame->child("camera");

  camera["position"]  = arcballCamera->eyePos();
  camera["direction"] = arcballCamera->lookDir();
  camera["up"]        = arcballCamera->upDir();
}

void GLFWSgWindow::motion(const vec2f &position)
{
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

    if (leftDown) {
      const vec2f mouseFrom(
          clamp(prev.x * 2.f / windowSize.x - 1.f, -1.f, 1.f),
          clamp(prev.y * 2.f / windowSize.y - 1.f, -1.f, 1.f));
      const vec2f mouseTo(clamp(mouse.x * 2.f / windowSize.x - 1.f, -1.f, 1.f),
                          clamp(mouse.y * 2.f / windowSize.y - 1.f, -1.f, 1.f));
      arcballCamera->rotate(mouseFrom, mouseTo);
    } else if (rightDown) {
      arcballCamera->zoom(mouse.y - prev.y);
    } else if (middleDown) {
      arcballCamera->pan(vec2f(mouse.x - prev.x, prev.y - mouse.y));
    }

    if (cameraChanged) {
      if (cancelFrameOnInteraction) {
        frame->cancelFrame();
        waitOnOSPRayFrame();
      }
      updateCamera();
    }
  }

  previousMouse = mouse;
}

void GLFWSgWindow::display()
{
  static auto displayStart = std::chrono::high_resolution_clock::now();

  if (showUi)
    buildUI();

  if (displayCallback)
    displayCallback(this);

  updateTitleBar();

  static bool firstFrame = true;
  if (firstFrame || frame->frameIsReady()) {
    // display frame rate in window title
    auto displayEnd = std::chrono::high_resolution_clock::now();
    auto durationMilliseconds =
        std::chrono::duration_cast<std::chrono::milliseconds>(displayEnd -
                                                              displayStart);

    latestFPS = 1000.f / float(durationMilliseconds.count());

    // map OSPRay frame buffer, update OpenGL texture with its contents, then
    // unmap

    waitOnOSPRayFrame();

    auto *fb =
        (void *)frame->mapFrame(showAlbedo ? OSP_FB_ALBEDO : OSP_FB_COLOR);

    const GLint glFormat = showAlbedo ? GL_RGB : GL_RGBA;
    const GLenum glType  = showAlbedo ? GL_FLOAT : GL_UNSIGNED_BYTE;
    glBindTexture(GL_TEXTURE_2D, framebufferTexture);
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 glFormat,
                 windowSize.x,
                 windowSize.y,
                 0,
                 glFormat,
                 glType,
                 fb);

    frame->unmapFrame(fb);

    // Start new frame and reset frame timing interval start
    displayStart = std::chrono::high_resolution_clock::now();
    startNewOSPRayFrame();
    firstFrame = false;
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
  }

  // swap buffers
  glfwSwapBuffers(glfwWindow);
}

void GLFWSgWindow::startNewOSPRayFrame()
{
  frame->startNewFrame();
}

void GLFWSgWindow::waitOnOSPRayFrame()
{
  frame->waitOnFrame();
}

void GLFWSgWindow::updateTitleBar()
{
  std::stringstream windowTitle;
  windowTitle << "OSPRay SG: " << std::setprecision(3) << latestFPS << " fps";
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

  glfwSetWindowTitle(glfwWindow, windowTitle.str().c_str());
}

void GLFWSgWindow::buildUI()
{
  ImGuiWindowFlags flags = ImGuiWindowFlags_AlwaysAutoResize;
  ImGui::Begin("press 'g' to hide/show UI", nullptr, flags);

  static int whichScene = 0;
  if (ImGui::Combo("scene##whichScene",
                   &whichScene,
                   sceneUI_callback,
                   nullptr,
                   g_scenes.size())) {
    scene = g_scenes[whichScene];
    refreshScene(true);
  }

  static int whichRenderer = 0;
  if (ImGui::Combo("renderer##whichRenderer",
                   &whichRenderer,
                   rendererUI_callback,
                   nullptr,
                   g_renderers.size())) {
    rendererTypeStr = g_renderers[whichRenderer];
    if (rendererTypeStr == "scivis")
      rendererType = OSPRayRendererType::SCIVIS;
    else if (rendererTypeStr == "pathtracer")
      rendererType = OSPRayRendererType::PATHTRACER;
    else
      rendererType = OSPRayRendererType::OTHER;
    refreshScene();
  }

  ImGui::Checkbox("cancel frame on interaction", &cancelFrameOnInteraction);
  ImGui::Checkbox("show albedo", &showAlbedo);

  ImGui::Separator();

  auto &renderer = frame->child("renderer");

  static int spp = 1;
  if (ImGui::SliderInt("spp", &spp, 1, 64))
    renderer["spp"] = spp;

  if (rendererType == OSPRayRendererType::PATHTRACER) {
    static int maxDepth = 20;
    if (ImGui::SliderInt("maxDepth", &maxDepth, 1, 64))
      renderer["maxDepth"] = maxDepth;

    static int rouletteDepth = 1;
    if (ImGui::SliderInt("rouletteDepth", &rouletteDepth, 1, 64))
      renderer["rouletteDepth"] = rouletteDepth;

    static float minContribution = 0.001f;
    if (ImGui::SliderFloat("minContribution", &minContribution, 0.f, 1.f))
      renderer["minContribution"] = minContribution;
  } else if (rendererType == OSPRayRendererType::SCIVIS) {
    static sg::rgba bgColor = renderer["bgColor"].valueAs<sg::rgba>();
    if (ImGui::ColorEdit4("bgColor", bgColor))
      renderer["bgColor"] = bgColor;

    static int aoSamples = 1;
    if (ImGui::SliderInt("aoSamples", &aoSamples, 0, 64))
      renderer["aoSamples"] = aoSamples;

    static float aoIntensity = 1.f;
    if (ImGui::SliderFloat("aoIntensity", &aoIntensity, 0.f, 1.f))
      renderer["aoIntensity"] = aoIntensity;
  }

  if (uiCallback) {
    ImGui::Separator();
    uiCallback();
  }

  ImGui::End();
}

void GLFWSgWindow::refreshScene(bool resetCamera)
{
  auto world = sg::createNode("world", "world");

  auto &g =
      world->createChildAs<sg::Generator>("generator", "generator_" + scene);
  g.generateData();

  world->render();

  frame->add(world);

  frame->createChild("renderer", "renderer_" + rendererTypeStr);

  if (resetCamera) {
    arcballCamera.reset(
        new ArcballCamera(frame->child("world").bounds(), windowSize));
    updateCamera();
  }
}
