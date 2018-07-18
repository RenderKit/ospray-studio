// ======================================================================== //
// Copyright 2016 SURVICE Engineering Company                               //
// Copyright 2016-2018 Intel Corporation                                    //
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
// ospcommon
#include "ospcommon/utility/getEnvVar.h"
#include "ospcommon/utility/SaveImage.h"
// ospray_sg
#include "sg/common/FrameBuffer.h"
#include "sg/generator/Generator.h"
#include "sg/visitor/GatherNodesByPosition.h"
#include "sg/visitor/MarkAllAsModified.h"
// imgui
#include "imgui.h"
#include "imguifilesystem/imguifilesystem.h"
// ospray_sg ui
#include "sg_ui/ospray_sg_ui.h"
// panels
#include "panels/About.h"
#include "panels/NodeFinder.h"
#include "panels/SGTreeView.h"

#include <GLFW/glfw3.h>

using namespace ospcommon;

static ImGuiFs::Dialog openFileDialog;

namespace ospray {

  MainWindow::MainWindow(const std::shared_ptr<sg::Frame> &scenegraph)
    : ImGui3DWidget(ImGui3DWidget::FRAMEBUFFER_NONE),
      scenegraph(scenegraph),
      renderer(scenegraph->child("renderer").nodeAs<sg::Renderer>()),
      renderEngine(scenegraph)
  {
    frameBufferMode = ImGui3DWidget::FRAMEBUFFER_UCHAR;

    auto OSPRAY_DYNAMIC_LOADBALANCER=
      utility::getEnvVar<int>("OSPRAY_DYNAMIC_LOADBALANCER");

    useDynamicLoadBalancer = OSPRAY_DYNAMIC_LOADBALANCER.value_or(false);

    if (useDynamicLoadBalancer)
      numPreAllocatedTiles = OSPRAY_DYNAMIC_LOADBALANCER.value();

    renderEngine.start();

    originalView = viewPort;

    scenegraph->child("frameAccumulationLimit") = accumulationLimit;

    // create panels //

    panels.emplace_back(new PanelNodeFinder(scenegraph));
    panels.emplace_back(new PanelSGTreeView(scenegraph));
    panels.emplace_back(new PanelAbout());
  }

  MainWindow::~MainWindow()
  {
    renderEngine.stop();
  }

  void MainWindow::mouseButton(int button, int action, int mods)
  {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS
        && ((mods & GLFW_MOD_SHIFT) | (mods & GLFW_MOD_CONTROL))) {
      const vec2f pos(currMousePos.x / static_cast<float>(windowSize.x),
                      1.f - currMousePos.y / static_cast<float>(windowSize.y));
      renderEngine.pick(pos);
      lastPickQueryType = (mods & GLFW_MOD_SHIFT) ? PICK_CAMERA : PICK_NODE;
    }
  }

  void MainWindow::reshape(const vec2i &newSize)
  {
    ImGui3DWidget::reshape(newSize);
    windowSize = newSize;

    viewPort.modified = true;

    renderEngine.setFbSize(newSize);
    scenegraph->child("frameBuffer")["size"].setValue(newSize);

    pixelBuffer.resize(newSize.x * newSize.y);
  }

  void MainWindow::keypress(char key)
  {
    switch (key) {
    case ' ':
    {
      if (renderer && renderer->hasChild("animationcontroller")) {
        bool animating =
            renderer->child("animationcontroller")["enabled"].valueAs<bool>();
        renderer->child("animationcontroller")["enabled"] = !animating;
      }
      break;
    }
    case 'R':
      toggleRenderingPaused();
      break;
    case '!':
      saveScreenshot("ospexampleviewer");
      break;
    case 'X':
      if (viewPort.up == vec3f(1,0,0) || viewPort.up == vec3f(-1.f,0,0)) {
        viewPort.up = - viewPort.up;
      } else {
        viewPort.up = vec3f(1,0,0);
      }
      viewPort.modified = true;
      break;
    case 'Y':
      if (viewPort.up == vec3f(0,1,0) || viewPort.up == vec3f(0,-1.f,0)) {
        viewPort.up = - viewPort.up;
      } else {
        viewPort.up = vec3f(0,1,0);
      }
      viewPort.modified = true;
      break;
    case 'Z':
      if (viewPort.up == vec3f(0,0,1) || viewPort.up == vec3f(0,0,-1.f)) {
        viewPort.up = - viewPort.up;
      } else {
        viewPort.up = vec3f(0,0,1);
      }
      viewPort.modified = true;
      break;
    case 'c':
      viewPort.modified = true;//Reset accumulation
      break;
    case 'o':
      showWindowImportData = true;
      break;
    case 'g':
      showWindowGenerateData = true;
      break;
    case 'r':
      resetView();
      break;
    case 'd':
      resetDefaultView();
      break;
    case 'p':
      printViewport();
      break;
    case '1':
      showWindowRenderStatistics = !showWindowRenderStatistics;
      break;
    case '2':
      showWindowJobStatusControlPanel = !showWindowJobStatusControlPanel;
      break;
    default:
      ImGui3DWidget::keypress(key);
    }
  }

  void MainWindow::resetView()
  {
    auto oldAspect = viewPort.aspect;
    viewPort = originalView;
    viewPort.aspect = oldAspect;
  }

  void MainWindow::resetDefaultView()
  {
    auto &world = renderer->child("world");
    auto bbox = world.bounds();
    vec3f diag = bbox.size();
    diag = max(diag, vec3f(0.3f * length(diag)));

    auto gaze = ospcommon::center(bbox);
    auto pos = gaze - .75f * vec3f(-.6 * diag.x, -1.2f * diag.y, .8f * diag.z);
    auto up = vec3f(0.f, 1.f, 0.f);

    auto &camera = scenegraph->child("camera");
    camera["pos"] = pos;
    camera["dir"] = normalize(gaze - pos);
    camera["up"] = up;

    setViewPort(pos, gaze, up);
    originalView = viewPort;
  }

  void MainWindow::printViewport()
  {
    printf("-vp %f %f %f -vu %f %f %f -vi %f %f %f\n",
           viewPort.from.x, viewPort.from.y, viewPort.from.z,
           viewPort.up.x,   viewPort.up.y,   viewPort.up.z,
           viewPort.at.x,   viewPort.at.y,   viewPort.at.z);
    fflush(stdout);
  }

  void MainWindow::saveScreenshot(const std::string &basename)
  {
    utility::writePPM(basename + ".ppm",
                      windowSize.x, windowSize.y, pixelBuffer.data());
    std::cout << "saved current frame to '" << basename << ".ppm'" << std::endl;
  }

  void MainWindow::toggleRenderingPaused()
  {
    renderingPaused = !renderingPaused;
    renderingPaused ? renderEngine.stop() : renderEngine.start();
  }

  void MainWindow::display()
  {
    if (renderEngine.hasNewPickResult()) {
      auto picked = renderEngine.getPickResult();
      if (picked.hit) {
        if (lastPickQueryType == PICK_NODE) {
#if 0
          sg::GatherNodesByPosition visitor((vec3f&)picked.position);
          scenegraph->traverse(visitor);
          collectedNodesFromSearch = visitor.results();
#else
          std::cout << "TODO: node picking currently not implemented!"
                    << std::endl;
#endif
        } else {
          // No conversion operator or ctor??
          viewPort.at.x = picked.position.x;
          viewPort.at.y = picked.position.y;
          viewPort.at.z = picked.position.z;
          viewPort.modified = true;
        }
      }
    }

    if (viewPort.modified) {
      auto &camera = scenegraph->child("camera");
      auto dir = viewPort.at - viewPort.from;
      if (camera.hasChild("focusdistance"))
        camera["focusdistance"] = length(dir);
      dir = normalize(dir);
      camera["dir"] = dir;
      camera["pos"] = viewPort.from;
      camera["up"]  = viewPort.up;
      camera.markAsModified();

      viewPort.modified = false;
    }

    if (renderEngine.hasNewFrame()) {
      auto &mappedFB = renderEngine.mapFramebuffer();
      size_t nPixels = windowSize.x * windowSize.y;

      if (mappedFB.size() == nPixels) {
        auto *srcPixels = mappedFB.data();
        auto *dstPixels = pixelBuffer.data();
        memcpy(dstPixels, srcPixels, nPixels * sizeof(uint32_t));
        lastFrameFPS = renderEngine.lastFrameFps();
        renderTime = 1.f/lastFrameFPS;
      }

      renderEngine.unmapFramebuffer();
    }

    ucharFB = pixelBuffer.data();
    ImGui3DWidget::display();

    lastTotalTime = ImGui3DWidget::totalTime;
    lastGUITime = ImGui3DWidget::guiTime;
    lastDisplayTime = ImGui3DWidget::displayTime;

    // that pointer is no longer valid, so set it to null
    ucharFB = nullptr;
  }

  void MainWindow::processFinishedJobs()
  {
    for (auto it = jobsInProgress.begin(); it != jobsInProgress.end(); ++it) {
      auto &job = *it;
      if (job.get() && job->isFinished()) {
        auto nodes = job->get();
        std::copy(nodes.begin(), nodes.end(), std::back_inserter(loadedNodes));
        jobsInProgress.erase(it++);
      }
    }
  }

  void MainWindow::clearScene()
  {
    bool wasRunning = renderEngine.runningState() == ExecState::RUNNING;
    renderEngine.stop();

    auto newWorld = sg::createNode("world", "Model");
    renderer->add(newWorld);

    scenegraph->traverse(sg::MarkAllAsModified{});

    if (wasRunning)
      renderEngine.start();
  }

  void MainWindow::buildGui()
  {
    ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse;

    processFinishedJobs();

    guiMainMenu();

    if (showWindowRenderStatistics) guiRenderStats();
    if (showWindowImportData) guiImportData();
    if (showWindowJobStatusControlPanel) guiJobStatusControlPanel();
    if (showWindowGenerateData) guiGenerateData();

    for (auto &p : panels)
      if (p->show) p->buildUI();

    if (showWindowImGuiDemo) ImGui::ShowTestWindow(&showWindowImGuiDemo);
  }

  void MainWindow::guiMainMenu()
  {
    if (ImGui::BeginMainMenuBar()) {
      guiMainMenuFile();
      guiMainMenuView();
      guiMainMenuCamera();
      guiMainMenuHelp();

      ImGui::EndMainMenuBar();
    }
  }

  void MainWindow::guiMainMenuFile()
  {
    if (ImGui::BeginMenu("File")) {

      if (ImGui::MenuItem("(o) Import Data..."))
        showWindowImportData = true;

      if (ImGui::MenuItem("(g) Generate Data..."))
        showWindowGenerateData = true;

      ImGui::Separator();
      ImGui::Separator();

      if (ImGui::MenuItem("Clear Scene"))
        clearScene();

      ImGui::Separator();
      ImGui::Separator();

      bool paused = renderingPaused;
      if (ImGui::Checkbox("Pause Rendering", &paused))
        toggleRenderingPaused();

      auto setFrameAccumulation = [&]() {
        accumulationLimit = accumulationLimit < 0 ? 0 : accumulationLimit;
        scenegraph->child("frameAccumulationLimit") = accumulationLimit;
      };

      if (ImGui::Checkbox("Limit Accumulation", &limitAccumulation)) {
        if (limitAccumulation)
          setFrameAccumulation();
      }

      if (limitAccumulation) {
        if(ImGui::InputInt("Frame Limit", &accumulationLimit))
          setFrameAccumulation();
      } else {
        scenegraph->child("frameAccumulationLimit") = -1;
      }

      ImGui::Separator();
      ImGui::Separator();

      if (ImGui::MenuItem("(q) Quit")) exitRequestedByUser = true;

      ImGui::EndMenu();
    }
  }

  void MainWindow::guiMainMenuView()
  {
    if (ImGui::BeginMenu("View")) {
      ImGui::Checkbox("(1) Rendering Stats", &showWindowRenderStatistics);
      ImGui::Checkbox("(2) Job Scheduler", &showWindowJobStatusControlPanel);

      ImGui::Separator();

      for (auto &p : panels)
        ImGui::Checkbox(p->name.c_str(), &p->show);

      ImGui::EndMenu();
    }
  }

  void MainWindow::guiMainMenuCamera()
  {
    if (ImGui::BeginMenu("Camera")) {

      ImGui::Checkbox("Auto-Rotate", &animating);

      ImGui::Separator();

      bool orbitMode = (manipulator == inspectCenterManipulator.get());
      bool flyMode   = (manipulator == moveModeManipulator.get());

      if (ImGui::Checkbox("Orbit Camera Mode", &orbitMode))
        manipulator = inspectCenterManipulator.get();

      if (orbitMode) ImGui::Checkbox("Anchor 'Up' Direction", &upAnchored);

      if (ImGui::Checkbox("Fly Camera Mode", &flyMode))
        manipulator = moveModeManipulator.get();

      if (ImGui::MenuItem("Reset View")) resetView();
      if (ImGui::MenuItem("Create Default View")) resetDefaultView();
      if (ImGui::MenuItem("Reset Accumulation")) viewPort.modified = true;
      if (ImGui::MenuItem("Print View")) printViewport();

      ImGui::InputFloat("Motion Speed", &motionSpeed);

      if (ImGui::MenuItem("Take Screenshot")) saveScreenshot("ospray_studio");

      ImGui::EndMenu();
    }
  }

  void MainWindow::guiMainMenuHelp()
  {
    if (ImGui::BeginMenu("Help")) {
      ImGui::Checkbox("Show ImGui Demo", &showWindowImGuiDemo);

      ImGui::EndMenu();
    }
  }

  void MainWindow::guiRenderStats()
  {
    auto flags = g_defaultWindowFlags |
                 ImGuiWindowFlags_NoResize |
                 ImGuiWindowFlags_AlwaysAutoResize;

    if (ImGui::Begin("Rendering Statistics",
                     &showWindowRenderStatistics,
                     flags)) {
      ImGui::Text("OSPRay render rate: %.1f fps", lastFrameFPS);
      ImGui::NewLine();
      ImGui::Text("Total GUI frame rate: %.1f fps", ImGui::GetIO().Framerate);
      ImGui::Text("Total 3dwidget time: %.1f ms", lastTotalTime*1000.f);
      ImGui::Text("GUI time: %.1f ms", lastGUITime*1000.f);
      ImGui::Text("display pixel time: %.1f ms", lastDisplayTime*1000.f);
      ImGui::NewLine();
      ImGui::Text("Variance: %.3f", renderEngine.getLastVariance());
    }

    ImGui::End();
  }

  void MainWindow::guiJobStatusControlPanel()
  {
    auto flags = g_defaultWindowFlags |
                 ImGuiWindowFlags_NoResize |
                 ImGuiWindowFlags_AlwaysAutoResize;

    if (ImGui::Begin("Job Scheduler Control Panel",
                     &showWindowJobStatusControlPanel,
                     flags)) {
      ImGui::Text("%i jobs running", jobsInProgress.size());
      ImGui::Text("%i nodes ready", loadedNodes.size());
      ImGui::NewLine();

      auto doIt = [&]() {
        bool wasRunning = renderEngine.runningState() == ExecState::RUNNING;
        renderEngine.stop();

        for (auto &node : loadedNodes)
          renderer->child("world").add(node);

        loadedNodes.clear();

        renderer->computeBounds();

        scenegraph->verify();

        resetDefaultView();
        resetView();

        setMotionSpeed(-1.f);
        setWorldBounds(renderer->child("bounds").valueAs<box3f>());

        if (wasRunning)
          renderEngine.start();
      };

      static bool autoImport = true;
      ImGui::Checkbox("auto add to scene", &autoImport);

      if (autoImport && !loadedNodes.empty())
        doIt();
      else if (ImGui::Button("Add Loaded Nodes to SceneGraph"))
        doIt();
    }

    ImGui::Separator();
    ImGui::Text("Loaded Nodes:");
    ImGui::NewLine();
    for (auto &n : loadedNodes)
      ImGui::Text(n->name().c_str());

    ImGui::End();
  }

  void MainWindow::guiGenerateData()
  {
    ImGui::OpenPopup("Generate Data");
    if (ImGui::BeginPopupModal("Generate Data",
                               &showWindowGenerateData,
                               ImGuiWindowFlags_AlwaysAutoResize)) {

      ImGui::Text("Generator Type:   ");
      ImGui::SameLine();

      static int which = 0;
      ImGui::Combo("##1", &which, "vtkWavelet\0cube\0spheres\0cylinders\0\0", 4);

      static std::string parameters;
      ImGui::Text("Generator Params: ");
      ImGui::SameLine();
      static std::array<char, 512> buf;
      strcpy(buf.data(), parameters.c_str());
      buf[parameters.size()] = '\0';
      if (ImGui::InputText("##2", buf.data(),
                           511)) {
        parameters = std::string(buf.data());
      }

      ImGui::Separator();

      if (ImGui::Button("OK", ImVec2(120,0))) {
        // TODO: move this inline-lambda to a named functor instead
        auto job = job_scheduler::schedule_job([=](){
          job_scheduler::Nodes retval;

          std::string type;

          switch (which) {
          case 0:
            type = "vtkWavelet";
            break;
          case 1:
            type = "cube";
            break;
          case 2:
            type = "spheres";
            break;
          case 3:
            type = "cylinders";
            break;
          default:
            std::cerr << "WAAAAT" << std::endl;
          }

          auto transformNode_ptr =
              sg::createNode("Transform_" + type, "Transform");

          auto generatorNode_ptr =
            sg::createNode("Generator_" + type,
                           "Generator")->nodeAs<sg::Generator>();
          transformNode_ptr->add(generatorNode_ptr);
          auto &generatorNode = *generatorNode_ptr;

          generatorNode["generatorType"] = type;
          generatorNode["parameters"] = parameters;

          generatorNode.generateData();

          retval.push_back(transformNode_ptr);

          return retval;
        });

        jobsInProgress.emplace_back(std::move(job));

        showWindowGenerateData = false;
        ImGui::CloseCurrentPopup();
      }

      ImGui::SameLine();

      if (ImGui::Button("Cancel", ImVec2(120,0))) {
        showWindowGenerateData = false;
        ImGui::CloseCurrentPopup();
      }

      ImGui::EndPopup();
    }
  }

  void MainWindow::guiImportData()
  {
    ImGui::OpenPopup("Import Data");
    if (ImGui::BeginPopupModal("Import Data",
                               &showWindowImportData,
                               ImGuiWindowFlags_AlwaysAutoResize)) {

      const bool pressed = ImGui::Button("Choose File...");
      const char *fileName = openFileDialog.chooseFileDialog(pressed);

      static std::string fileToOpen;

      if (strlen(fileName) > 0)
        fileToOpen = fileName;

      ImGui::Text("File:");
      ImGui::SameLine();

      std::array<char, 512> buf;
      strcpy(buf.data(), fileToOpen.c_str());

      ImGui::InputText("", buf.data(), buf.size(),
                       ImGuiInputTextFlags_EnterReturnsTrue);

      fileToOpen = buf.data();

      ImGui::Separator();

      if (ImGui::Button("OK", ImVec2(120,0))) {
        // TODO: move this inline-lambda to a named functor instead
        auto job = job_scheduler::schedule_job([=](){
          job_scheduler::Nodes retval;

          auto transformNode_ptr =
              sg::createNode("Transform_" + fileToOpen, "Transform");

          auto importerNode_ptr =
              sg::createNode("Importer_" + fileToOpen,
                             "Importer")->nodeAs<sg::Importer>();
          transformNode_ptr->add(importerNode_ptr);
          auto &importerNode = *importerNode_ptr;

          try {
            importerNode["fileName"] = fileToOpen;
            importerNode.setChildrenModified(sg::TimeStamp());// trigger load
          } catch (...) {
            std::cerr << "Failed to open file '" << fileToOpen << "'!\n";
          }

          retval.push_back(transformNode_ptr);

          return retval;
        });

        jobsInProgress.emplace_back(std::move(job));

        showWindowImportData = false;
        ImGui::CloseCurrentPopup();
      }

      ImGui::SameLine();

      if (ImGui::Button("Cancel", ImVec2(120,0))) {
        showWindowImportData = false;
        ImGui::CloseCurrentPopup();
      }

      ImGui::EndPopup();
    }
  }

  void MainWindow::setCurrentDeviceParameter(const std::string &param,
                                             int value)
  {
    renderEngine.stop();

    auto device = ospGetCurrentDevice();
    if (device == nullptr)
      throw std::runtime_error("FATAL: could not get current OSPDevice!");

    ospDeviceSet1i(device, param.c_str(), value);
    ospDeviceCommit(device);

    if (!renderingPaused)
      renderEngine.start();
  }

} // ::ospray
