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

#pragma once

#include "ArcballCamera.h"
// glfw
#include "GLFW/glfw3.h"
// ospray
#include "sg/Frame.h"
// std
#include <functional>
#include "sg/renderer/MaterialRegistry.h"

using namespace ospcommon::math;
using namespace ospray;

enum class OSPRayRendererType
{
  SCIVIS,
  PATHTRACER,
  DEBUGGER,
  OTHER
};

class MainWindow
{
 public:
  MainWindow(const vec2i &windowSize);

  ~MainWindow();

  static MainWindow *getActiveWindow();

  void registerDisplayCallback(std::function<void(MainWindow *)> callback);

  void registerImGuiCallback(std::function<void()> callback);

  void mainLoop();

  void parseCommandLine(int &ac, const char **&av);
  void importFiles();

 protected:
  void updateCamera();

  void reshape(const vec2i &newWindowSize);
  void motion(const vec2f &position);
  void display();
  void startNewOSPRayFrame();
  void waitOnOSPRayFrame();
  void updateTitleBar();
  void buildUI();
  void refreshRenderer();
  void refreshScene();
  void refreshMaterialRegistry();
  void refreshLight();
  void refreshMaterial();
  void addPTMaterials();

  static MainWindow *activeWindow;

  vec2i windowSize;
  vec2i fbSize;
  vec2f previousMouse{-1.f};

  bool showAlbedo{false};
  bool cancelFrameOnInteraction{false};

  std::string scene;
  std::vector<std::string> filesToImport;

  OSPRayRendererType rendererType{OSPRayRendererType::SCIVIS};
  std::string rendererTypeStr{"scivis"};
  std::string lightTypeStr{"ambient"};
  std::string matTypeStr{"material_default"};

  // GLFW window instance
  GLFWwindow *glfwWindow = nullptr;

  // Arcball camera instance
  std::unique_ptr<ArcballCamera> arcballCamera;

  std::shared_ptr<sg::Frame> frame;
  sg::MaterialRegistry mr;

  // OpenGL framebuffer texture
  GLuint framebufferTexture = 0;

  // optional registered display callback, called before every display()
  std::function<void(MainWindow *)> displayCallback;

  // toggles display of ImGui UI, if an ImGui callback is provided
  bool showUi = true;

  // optional registered ImGui callback, called during every frame to build UI
  std::function<void()> uiCallback;

  // FPS measurement of last frame
  float latestFPS{0.f};
};
