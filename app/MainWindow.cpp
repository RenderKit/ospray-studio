// ======================================================================== //
// Copyright 2018-2020 Intel Corporation                                    //
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
#include "sg/generator/Generator.h"
#include "sg/importer/Importer.h"
#include "sg/exporter/Exporter.h"
#include "sg/visitors/GenerateImGuiWidgets.h"
#include "sg/visitors/PrintNodes.h"
#include "sg/scene/lights/Lights.h"
// rkcommon
#include "rkcommon/math/ospmath.h"
#include "rkcommon/os/FileName.h"
#include "rkcommon/utility/SaveImage.h"
#include "rkcommon/utility/getEnvVar.h"
// tiny_file_dialogs
#include "sg/scene/volume/Structured.h"
#include "tinyfiledialogs.h"

// GUI mode entry point
void start_GUI_mode(int argc, const char *argv[])
{
  std::cerr << "GUI mode\n";

  bool denoiser = ospLoadModule("denoiser") == OSP_NO_ERROR;

  auto window = make_unique<MainWindow>(vec2i(1024, 768), denoiser);
  window->parseCommandLine(argc, argv);
  window->mainLoop();
  window.reset();
}

static ImGuiWindowFlags g_imguiWindowFlags = ImGuiWindowFlags_AlwaysAutoResize;

static bool g_quitNextFrame = false;
static bool g_saveNextFrame = false;
static bool g_animatingPath = false;

static const std::vector<std::string> g_scenes = {"multilevel_hierarchy",
                                                  "torus",
                                                  "texture_volume_test",
                                                  "tutorial_scene",
                                                  "random_spheres",
                                                  "wavelet",
                                                  "import volume",
                                                  "import",
                                                  "unstructured_volume"};

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
    "ambient", "distant", "spot", "sphere", "quad"};

std::vector<std::string> g_matTypes = {
    "obj", "alloy", "glass", "carPaint", "luminous", "metal", "thinGlass"};

std::vector<CameraState> g_camAnchors;  // user-defined anchor states
std::vector<CameraState> g_camPath;     // interpolated path through anchors
int g_camSelectedAnchorIndex = 0;
int g_camCurrentPathIndex    = 0;
int g_camPathSpeed = 5;  // defined in hundredths (e.g. 10 = 10 * 0.01 = 0.1)
const int g_camPathPause = 2;  // _seconds_ to pause for at end of path

std::string quatToString(quaternionf &q)
{
  std::stringstream ss;
  ss << q;
  return ss.str();
}

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

  glfwSetKeyCallback(
      glfwWindow, [](GLFWwindow *, int key, int, int action, int mod) {
      auto &io = ImGui::GetIO();
        if (!io.WantCaptureKeyboard && action == GLFW_PRESS) {
          switch (key) {
          case GLFW_KEY_G:
            activeWindow->showUi = !(activeWindow->showUi);
            break;
          case GLFW_KEY_Q: {
            auto showMode =
                ospcommon::utility::getEnvVar<int>("OSPSTUDIO_SHOW_MODE");
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
          case GLFW_KEY_S:
            g_saveNextFrame = true;
            break;
          case GLFW_KEY_SPACE:
            g_animatingPath       = !g_animatingPath;
            g_camCurrentPathIndex = 0;
            if (g_animatingPath) {
              g_camPath = buildPath(g_camAnchors, g_camPathSpeed * 0.01);
            }
            break;
          }
        }
      });

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
  refreshScene();

  // trigger window reshape events with current window size
  glfwGetFramebufferSize(glfwWindow, &this->windowSize.x, &this->windowSize.y);
  reshape(this->windowSize);
  fbSize = this->windowSize;
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

void MainWindow::registerDisplayCallback(
    std::function<void(MainWindow *)> callback)
{
  displayCallback = callback;
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

void MainWindow::updateCamera()
{
  auto &camera = frame->child("camera");

  camera["position"]  = arcballCamera->eyePos();
  camera["direction"] = arcballCamera->lookDir();
  camera["up"]        = arcballCamera->upDir();
}

void MainWindow::motion(const vec2f &position)
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
      arcballCamera->rotate(mouseFrom, lerp(sensitivity, mouseFrom, mouseTo));
    } else if (rightDown) {
      arcballCamera->zoom((mouse.y - prev.y) * sensitivity);
    } else if (middleDown) {
      arcballCamera->pan(vec2f(mouse.x - prev.x, prev.y - mouse.y) *
                         sensitivity);
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

  if (showUi)
    buildUI();

  if (displayCallback)
    displayCallback(this);

  updateTitleBar();

  static bool firstFrame = true;
  if (firstFrame || (frame->frameIsReady() && fbSize == windowSize)) {
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

    // This needs to query the actual framebuffer format
    const GLint glFormat = showAlbedo ? GL_RGB : GL_RGBA;
    // const GLenum glType  = (showAlbedo || screenshotFiletype == ImageType::HDR)
    //                          ? GL_FLOAT
    //                          : GL_UNSIGNED_BYTE;
    const GLenum glType = GL_FLOAT;  // required for denoiser
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

    if (g_saveNextFrame) {
      saveCurrentFrame();
      g_saveNextFrame = false;
    }

    // Start new frame and reset frame timing interval start
    displayStart = std::chrono::high_resolution_clock::now();
    startNewOSPRayFrame();
    firstFrame = false;
  } else if (fbSize != windowSize) {
    waitOnOSPRayFrame();
    startNewOSPRayFrame();
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

void MainWindow::startNewOSPRayFrame()
{
  frame->startNewFrame();
  fbSize = windowSize;
}

void MainWindow::waitOnOSPRayFrame()
{
  frame->waitOnFrame();
}

void MainWindow::updateTitleBar()
{
  std::stringstream windowTitle;
  windowTitle << "OSPRay Studio: " << std::setprecision(3) << latestFPS
              << " fps";
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

void MainWindow::buildUI()
{
  // build main menu and options
  buildMainMenu();

  // build window UIs as needed
  buildWindows();

  ImGui::Begin("press 'g' to hide/show UI", nullptr, g_imguiWindowFlags);

  static int whichScene        = 0;
  static int whichRenderer     = 0;
  static int whichDebuggerType = 0;
  static int whichLightType    = 0;
  static int whichMatType      = 0;
  if (ImGui::Combo("scene##whichScene",
                   &whichScene,
                   sceneUI_callback,
                   nullptr,
                   g_scenes.size())) {
    scene = g_scenes[whichScene];

    auto numImportedMats = baseMaterialRegistry->matImportsList.size();
    g_matTypes.erase(g_matTypes.begin(), g_matTypes.begin() + numImportedMats);
    baseMaterialRegistry->rmMatImports();

    whichMatType = 0;  // necessary to reset menu material name on scene change

    refreshRenderer();
    refreshScene();
  }

  if (ImGui::Combo("renderer##whichRenderer",
                   &whichRenderer,
                   rendererUI_callback,
                   nullptr,
                   g_renderers.size())) {
    rendererTypeStr = g_renderers[whichRenderer];

    if (rendererType == OSPRayRendererType::DEBUGGER)
      whichDebuggerType = 0;  // reset UI if switching away from debug renderer

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

  auto &renderer = frame->child("renderer");

  int spp = renderer["pixelSamples"].valueAs<int>();
  if (ImGui::SliderInt("pixelSamples", &spp, 1, 64))
    renderer["pixelSamples"] = spp;

  sg::rgba bgColor = renderer["backgroundColor"].valueAs<sg::rgba>();
  if (ImGui::ColorEdit4("backgroundColor", bgColor))
    renderer["backgroundColor"] = bgColor;

  if (rendererType == OSPRayRendererType::PATHTRACER) {
    int maxDepth = renderer["maxPathLength"].valueAs<int>();
    if (ImGui::SliderInt("maxPathLength", &maxDepth, 1, 64))
      renderer["maxPathLength"] = maxDepth;

    int rouletteDepth = renderer["roulettePathLength"].valueAs<int>();
    if (ImGui::SliderInt("roulettePathLength", &rouletteDepth, 1, 64))
      renderer["roulettePathLength"] = rouletteDepth;
  } else if (rendererType == OSPRayRendererType::SCIVIS) {
    int aoSamples = renderer["aoSamples"].valueAs<int>();
    if (ImGui::SliderInt("aoSamples", &aoSamples, 0, 64))
      renderer["aoSamples"] = aoSamples;

    float aoIntensity = renderer["aoIntensity"].valueAs<float>();
    if (ImGui::SliderFloat("aoIntensity", &aoIntensity, 0.f, 1.f))
      renderer["aoIntensity"] = aoIntensity;
  }

  if (rendererType == OSPRayRendererType::DEBUGGER) {
    if (ImGui::Combo("debug type##whichDebugType",
                     &whichDebuggerType,
                     debugTypeUI_callback,
                     nullptr,
                     g_debugRendererTypes.size())) {
      renderer["method"] = g_debugRendererTypes[whichDebuggerType];
    }
  } else if (rendererType == OSPRayRendererType::PATHTRACER) {
    if (ImGui::Combo("light type##whichLightType",
                     &whichLightType,
                     lightTypeUI_callback,
                     nullptr,
                     g_lightTypes.size())) {
      lightTypeStr = g_lightTypes[whichLightType];
      refreshLight();
    }
    if (ImGui::Combo("material type##whichMatType",
                     &whichMatType,
                     matTypeUI_callback,
                     nullptr,
                     g_matTypes.size())) {
      matTypeStr = g_matTypes[whichMatType];
      refreshMaterial();
    }
  } else if (rendererType == OSPRayRendererType::SCIVIS) {
    if (ImGui::Checkbox("backplate texture", &useTestTex) ||
        ImGui::Checkbox("import backplate texture", &useImportedTex)) {
      refreshRenderer();
    }
  }

  ImGui::Checkbox("cancel frame on interaction", &cancelFrameOnInteraction);
  ImGui::Checkbox("show albedo", &showAlbedo);

  if (denoiserAvailable) {
    if (ImGui::Checkbox("denoiser", &frame->denoiserEnabled))
      frame->updateFrameOpsNextFrame = true;
  }

  ImGui::Checkbox("auto rotate", &autorotate);
  if (autorotate) {
    ImGui::SliderInt("auto rotate speed", &autorotateSpeed, 1, 100);
  }

  ImGui::Separator();

  if (uiCallback) {
    ImGui::Separator();
    uiCallback();
  }

  ImGui::End();
}

void MainWindow::refreshRenderer()
{
  auto &r = frame->createChild("renderer", "renderer_" + rendererTypeStr);
  if (rendererTypeStr != "debug") {
    baseMaterialRegistry->updateMaterialList(rendererTypeStr);
    r.createChildData("material", baseMaterialRegistry->cppMaterialList);
  }
  // backplate does not work without renderer pre/post commit
  // which cause issues with material textures
  // uncomment scivis pre/post commit to see this working
  // TODO:: fix this issue
  if (rendererTypeStr == "scivis") {
    if (useTestTex) {
      auto &backplateTex = r.createChild("map_backplate", "texture_2d");
      std::vector<vec4f> backplate;
      backplate.push_back(vec4f(0.8f, 0.2f, 0.2f, 1.0f));
      backplate.push_back(vec4f(0.2f, 0.8f, 0.2f, 1.0f));
      backplate.push_back(vec4f(0.2f, 0.2f, 0.8f, 1.0f));
      backplate.push_back(vec4f(0.4f, 0.2f, 0.4f, 1.0f));
      backplateTex.createChild("format", "int", 2);
      backplateTex.createChildData("data", vec2ul(2, 2), backplate.data());
    } else if (useImportedTex) {
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

void MainWindow::refreshLight()
{
  auto &world = frame->child("world");
  world.createChild("light", lightTypeStr);
}

void MainWindow::refreshMaterial()
{
  baseMaterialRegistry->refreshMaterialList(matTypeStr, rendererTypeStr);
  auto &r = frame->child("renderer");
  r.createChildData("material", baseMaterialRegistry->cppMaterialList);
}

void MainWindow::refreshScene()
{
  auto world = sg::createNode("world", "world");

  world->createChild("materialref", "reference_to_material", 0);

  if (scene == "import") {
    if (!importGeometry(world)) {
      return;
    }
  } else if (scene == "import volume") {
    if (!importVolume(world)) {
      return;
    }
  } else {
    auto &gen =
        world->createChildAs<sg::Generator>("generator", "generator_" + scene);
    gen.generateData();
  }
  world->createChild("light", lightTypeStr);

  world->render();

  frame->add(world);

  arcballCamera.reset(
      new ArcballCamera(frame->child("world").bounds(), windowSize));
  updateCamera();
}

void MainWindow::parseCommandLine(int &ac, const char **&av)
{
  for (int i = 1; i < ac; i++) {
    const std::string arg = av[i];

    filesToImport.push_back(arg);
  }

  if (!filesToImport.empty()) {
    importFiles();
  }
}

void MainWindow::importFiles()
{
  auto world = sg::createNode("world", "world");

  for (auto file : filesToImport) {
//    try {
      rkcommon::FileName fileName(file);
      std::string nodeName = fileName.base() + "_importer";

      std::cout << "Importing: " << file << std::endl;
      auto oldRegistrySize = baseMaterialRegistry->children().size();
      auto importer        = sg::getImporter(file);
      if (importer != "") {
        auto &imp   = world->createChildAs<sg::Importer>(nodeName, importer);
        imp["file"] = std::string(file);
        imp.importScene(baseMaterialRegistry);

#if 0  // BMCDEBUG, just for testing
        imp.traverse<sg::PrintNodes>();
#endif

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
    //    } catch (...) {
    //      std::cerr << "Failed to open file '" << file << "'!\n";
    //    }
  }
  world->createChild("light", lightTypeStr);

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
    std::shared_ptr<sg::StructuredVolume> volumeImport =
        std::static_pointer_cast<sg::StructuredVolume>(
            sg::createNode("volume", "volume_structured"));

    volumeImport->load(file);
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
  if (screenshotFiletype == "exr")
    frame->saveFrame(filename, screenshotDepth);
  else
    frame->saveFrame(filename);
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
    if (ImGui::BeginMenu("Import")) {
      if (ImGui::MenuItem("Import geometry...", nullptr)) {
        scene = "import";
        refreshScene();
      }
      if (ImGui::MenuItem("Import volume...", nullptr)) {
        scene = "import volume";
        refreshScene();
      }
      ImGui::EndMenu();
    }
    ImGui::EndMenu();
  }
}

void MainWindow::buildMainMenuEdit()
{
  if (ImGui::BeginMenu("Edit")) {
    if (ImGui::MenuItem("Lights...", "", nullptr))
      showLightEditor = true;
    if (ImGui::MenuItem("Preferences...", nullptr))
      showPreferences = true;
    ImGui::EndMenu();
  }
}

void MainWindow::buildMainMenuView()
{
  if (ImGui::BeginMenu("View")) {
    if (ImGui::MenuItem("Screenshot", "s", nullptr))
      g_saveNextFrame = true;
    if (ImGui::MenuItem("Keyframes...", "", nullptr))
      showKeyframes = true;
    ImGui::EndMenu();
  }
}

// Option windows /////////////////////////////////////////////////////////////

void MainWindow::buildWindows()
{
  if (showPreferences)
    buildWindowPreferences();
  if (showKeyframes)
    buildWindowKeyframes();
  if (showLightEditor)
    buildWindowLightEditor();
}

void MainWindow::buildWindowPreferences()
{
  if (!ImGui::Begin("Preferences", &showPreferences, g_imguiWindowFlags)) {
    ImGui::End();
    return;
  }

  // are these names too terse?
  static const std::vector<std::string> screenshotFiletypes =
      sg::getExporterTypes();

  static int screenshotFiletypeChoice = std::distance(
      screenshotFiletypes.begin(),
      std::find(screenshotFiletypes.begin(), screenshotFiletypes.end(), "png"));

  if (ImGui::Combo("Screenshot filetype",
                   (int *)&screenshotFiletypeChoice,
                   stringVec_callback,
                   (void *)screenshotFiletypes.data(),
                   screenshotFiletypes.size())) {
    screenshotFiletype = screenshotFiletypes[screenshotFiletypeChoice];
  }

  if (screenshotFiletype == "exr") {
    ImGui::Checkbox("Include depth buffer", &screenshotDepth);
  }

  ImGui::End();
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
  }
  ImGui::End();
}

void MainWindow::buildWindowLightEditor()
{
  if (!ImGui::Begin("Light editor", &showLightEditor, g_imguiWindowFlags)) {
    ImGui::End();
    return;
  }

  static int whichLightType = -1;
  ImGui::Combo("light type##whichLightType",
               &whichLightType,
               lightTypeUI_callback,
               nullptr,
               g_lightTypes.size());

  static bool lightNameWarning = false;
  static char lightName[64] = "";
  if (ImGui::InputText("name", lightName, 64))
    lightNameWarning = false;

  auto &world    = frame->child("world");
  auto &lightMan = world.childAs<sg::Lights>("lights");
  auto &lights   = lightMan.children();

  static int whichLight = -1;
  static std::string selectedLight;
  std::string lightType = g_lightTypes[whichLightType];

  if (lightNameWarning)
    ImGui::TextColored(ImVec4(1.f, 0.f, 0.f, 1.f),
                       "Light must have unique non-empty name");

  if (ImGui::Button("add")) {
    if (whichLightType != -1) {
      if (!lightMan.addLight(lightName, lightType)) {
        lightNameWarning = true;
      }
    }
  }

  if (lights.size() > 1) {
    ImGui::SameLine();
    if (ImGui::Button("remove")) {
      if (whichLight != -1) {
        lightMan.removeLight(selectedLight);
        whichLight    = std::max(0, whichLight - 1);
        selectedLight = (*(lights.begin() + whichLight)).first;
      }
    }
  }

  if (ImGui::ListBoxHeader("lights", 3)) {
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
      ImGui::Separator();
      ImGui::Text("Edit light");
      lightMan.child(selectedLight).traverse<sg::GenerateImGuiWidgets>();
    }
  }

  ImGui::End();
}
