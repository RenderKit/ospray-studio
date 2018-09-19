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
    : ImGui3DWidget(ImGui3DWidget::RESIZE_KEEPFOVY),
      scenegraph(scenegraph),
      renderer(scenegraph->child("renderer").nodeAs<sg::Renderer>()),
      renderEngine(scenegraph)
  {
    auto &navFB = scenegraph->createChild("navFrameBuffer", "FrameBuffer");
    navFB["useAccumBuffer"] = false;
    navFB["useVarianceBuffer"] = false;

    auto OSPRAY_DYNAMIC_LOADBALANCER=
      utility::getEnvVar<int>("OSPRAY_DYNAMIC_LOADBALANCER");

    useDynamicLoadBalancer = OSPRAY_DYNAMIC_LOADBALANCER.value_or(false);

    if (useDynamicLoadBalancer)
      numPreAllocatedTiles = OSPRAY_DYNAMIC_LOADBALANCER.value();

    originalView = viewPort;

    scenegraph->child("frameAccumulationLimit") = accumulationLimit;

    ospSetProgressFunc(&ospray::MainWindow::progressCallbackWrapper, this);

    // create panels //

    panels.emplace_back(new PanelNodeFinder(scenegraph));
    panels.emplace_back(new PanelSGTreeView(scenegraph));
    panels.emplace_back(new PanelAbout());
  }

  MainWindow::~MainWindow()
  {
    renderEngine.stop();
    ospSetProgressFunc(nullptr, nullptr);
  }

  void MainWindow::startAsyncRendering()
  {
    renderEngine.start();
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
    scenegraph->child("frameBuffer")["size"] = newSize;
    scenegraph->child("navFrameBuffer")["size"] = navRenderSize;
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
      saveScreenshot = true;
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
    viewPort.modified = true;
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
#if 0 // TODO: this list needs to be forwarded to a UI widget (or pick pos?)
          sg::GatherNodesByPosition visitor((vec3f&)picked.position);
          scenegraph->traverse(visitor);
          collectedNodesFromSearch = visitor.results();
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

      // don't cancel the first frame, otherwise it is hard to navigate
      if (scenegraph->frameId() > 0 && cancelFrameOnInteraction) {
        cancelRendering = true;
        renderEngine.setFrameCancelled();
      }

      viewPort.modified = false;
    }

    renderFPS = renderEngine.lastFrameFps();
    renderFPSsmoothed = renderEngine.lastFrameFpsSmoothed();

    if (renderEngine.hasNewFrame()) {
      auto &mappedFB = renderEngine.mapFramebuffer();
      auto fbSize = mappedFB.size();
      auto fbData = mappedFB.data();
      GLenum texelType;
      std::string filename("ospexampleviewer");
      switch (mappedFB.format()) {
        default: /* fallthrough */
        case OSP_FB_NONE:
          fbData = nullptr;
          break;
        case OSP_FB_RGBA8: /* fallthrough */
        case OSP_FB_SRGBA:
          texelType = GL_UNSIGNED_BYTE;
          if (saveScreenshot) {
            filename += ".ppm";
            utility::writePPM(filename, fbSize.x, fbSize.y, (uint32_t*)fbData);
          }
          break;
        case OSP_FB_RGBA32F:
          texelType = GL_FLOAT;
          if (saveScreenshot) {
            filename += ".pfm";
            utility::writePFM(filename, fbSize.x, fbSize.y, (vec4f*)fbData);
          }
          break;
      }

      // update/upload fbTexture
      if (fbData) {
        fbAspect = fbSize.x/float(fbSize.y);
        glBindTexture(GL_TEXTURE_2D, fbTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, fbSize.x, fbSize.y, 0, GL_RGBA,
            texelType, fbData);
      } else
        fbAspect = 1.f;

      if (saveScreenshot) {
        std::cout << "saved current frame to '" << filename << "'" << std::endl;
        saveScreenshot = false;
      }

      renderEngine.unmapFramebuffer();
    }

    // set border color TODO maybe move to application
    vec4f texBorderCol(0.f); // default black
    // TODO be more sophisticated (depending on renderer type, fb mode (sRGB))
    if (renderer->child("useBackplate").valueAs<bool>()) {
      auto col = renderer->child("bgColor").valueAs<vec3f>();
      const float g = 1.f/2.2f;
      texBorderCol = vec4f(powf(col.x, g), powf(col.y, g), powf(col.z, g), 0.f);
    }
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, &texBorderCol[0]);

    ImGui3DWidget::display();

    lastTotalTime = ImGui3DWidget::totalTime;
    lastGUITime = ImGui3DWidget::guiTime;
    lastDisplayTime = ImGui3DWidget::displayTime;
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

      if (ImGui::BeginMenu("Scale Resolution")) {
        float scale = renderResolutionScale;
        if (ImGui::MenuItem("0.25x")) renderResolutionScale = 0.25f;
        if (ImGui::MenuItem("0.50x")) renderResolutionScale = 0.5f;
        if (ImGui::MenuItem("0.75x")) renderResolutionScale = 0.75f;

        ImGui::Separator();

        if (ImGui::MenuItem("1.00x")) renderResolutionScale = 1.f;

        ImGui::Separator();

        if (ImGui::MenuItem("1.25x")) renderResolutionScale = 1.25f;
        if (ImGui::MenuItem("2.00x")) renderResolutionScale = 2.0f;
        if (ImGui::MenuItem("4.00x")) renderResolutionScale = 4.0f;

        ImGui::Separator();

        if (ImGui::BeginMenu("custom")) {
          ImGui::InputFloat("x##fb_scaling", &renderResolutionScale);
          ImGui::EndMenu();
        }

        if (scale != renderResolutionScale)
          reshape(windowSize);

        ImGui::EndMenu();
      }

      if (ImGui::BeginMenu("Scale Resolution while Navigating")) {
        float scale = navRenderResolutionScale;
        if (ImGui::MenuItem("0.25x")) navRenderResolutionScale = 0.25f;
        if (ImGui::MenuItem("0.50x")) navRenderResolutionScale = 0.5f;
        if (ImGui::MenuItem("0.75x")) navRenderResolutionScale = 0.75f;

        ImGui::Separator();

        if (ImGui::MenuItem("1.00x")) navRenderResolutionScale = 1.f;

        ImGui::Separator();

        if (ImGui::MenuItem("1.25x")) navRenderResolutionScale = 1.25f;
        if (ImGui::MenuItem("2.00x")) navRenderResolutionScale = 2.0f;
        if (ImGui::MenuItem("4.00x")) navRenderResolutionScale = 4.0f;

        ImGui::Separator();

        if (ImGui::BeginMenu("custom")) {
          ImGui::InputFloat("x##fb_scaling", &navRenderResolutionScale);
          ImGui::EndMenu();
        }

        if (scale != navRenderResolutionScale)
          reshape(windowSize);

        ImGui::EndMenu();
      }

      if (ImGui::BeginMenu("Aspect Control")) {
        const float aspect = fixedRenderAspect;
        if (ImGui::MenuItem("Lock"))
          fixedRenderAspect = (float)windowSize.x / windowSize.y;
        if (ImGui::MenuItem("Unlock")) fixedRenderAspect = 0.f;
        ImGui::InputFloat("Set", &fixedRenderAspect);
        fixedRenderAspect = std::max(fixedRenderAspect, 0.f);

        if (aspect != fixedRenderAspect) {
          if (fixedRenderAspect > 0.f)
            resizeMode = RESIZE_LETTERBOX;
          else
            resizeMode = RESIZE_KEEPFOVY;
          reshape(windowSize);
        }

        ImGui::EndMenu();
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
      ImGui::NewLine();
      if (renderFPS > 1.f) {
        ImGui::Text("OSPRay render rate: %.1f fps", renderFPS);
      } else {
        ImGui::Text("OSPRay render time: %.1f sec. Frame progress: ", 1.f/renderFPS);
        ImGui::SameLine();
        ImGui::ProgressBar(frameProgress);
      }
      ImGui::Text("  Total GUI frame rate: %.1f fps", ImGui::GetIO().Framerate);
      ImGui::Text("  Total 3dwidget time: %.1f ms", lastTotalTime*1000.f);
      ImGui::Text("  GUI time: %.1f ms", lastGUITime*1000.f);
      ImGui::Text("  display pixel time: %.1f ms", lastDisplayTime*1000.f);
      auto variance = renderer->getLastVariance();
      ImGui::Text("Variance: %.3f", variance);

      auto eta = scenegraph->estimatedSeconds();
      if (std::isfinite(eta)) {
        auto sec = scenegraph->elapsedSeconds();
        ImGui::SameLine();
        ImGui::Text(" Total progress: ");

        char str[100];
        if (sec < eta)
          snprintf(str, sizeof(str), "%.1f s / %.1f s", sec, eta);
        else
          snprintf(str, sizeof(str), "%.1f s", sec);

        ImGui::SameLine();
        ImGui::ProgressBar(sec/eta, ImVec2(-1,0), str);
      }
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

  int MainWindow::progressCallback(const float progress)
  {
    frameProgress = progress;

    // one-shot cancel
    return !cancelRendering.exchange(false);
  }

  int MainWindow::progressCallbackWrapper(void * ptr, const float progress)
  {
    return ((MainWindow*)ptr)->progressCallback(progress);
  }

} // ::ospray
