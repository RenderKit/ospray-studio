// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// MainWindow subclasses includes
#include "GUIContext.h"
#include "MainWindow.h"

// std
#include <iostream>
#include <stdexcept>
// ospray_sg
#include "sg/Math.h"
#include "sg/camera/Camera.h"
#include "sg/fb/FrameBuffer.h"
#include "sg/generator/Generator.h"
#include "sg/renderer/Renderer.h"
#include "sg/scene/World.h"
#include "sg/scene/lights/LightsManager.h"
#include "sg/scene/volume/Volume.h"
#include "sg/visitors/CollectTransferFunctions.h"
#include "sg/visitors/Commit.h"
#include "sg/visitors/PrintNodes.h"
#include "sg/visitors/Search.h"

#include <fstream>
#include <queue>

// CLI
#include <CLI11.hpp>

using namespace ospray;

MainWindow *GUIContext::mainWindow = nullptr;

// GUIContext definitions ///////////////////////////////////////////////

CameraMap GUIContext::g_sceneCameras;

GUIContext::GUIContext(StudioCommon &_common)
    : StudioContext(_common, StudioMode::GUI)
{
  pluginManager = std::make_shared<PluginManager>();
  optSPP = 1; // Default SamplesPerPixel in interactive mode is one.
  if (frame->hasChild("framebuffer"))
  framebuffer = frame->child("framebuffer").nodeAs<sg::FrameBuffer>();
  windowSize = _common.defaultSize;
  cameraStack = std::make_shared<std::vector<CameraState>>();
}

GUIContext::~GUIContext()
{
  pluginManager->removeAllPlugins();
  g_sceneCameras.clear();
  sg::clearAssets();
}

void GUIContext::start()
{
  std::cerr << "GUI mode\n";
  currentUtil = std::shared_ptr<GUIContext>(this);
  if (!mainWindow) {
    mainWindow = new MainWindow(windowSize, currentUtil);
    mainWindow->cameraStack = cameraStack;
    mainWindow->initGLFW();
  }

  // load plugins //
  for (auto &p : studioCommon.pluginsToLoad)
    pluginManager->loadPlugin(p);

  // create panels //
  // doing this outside constructor to ensure shared_from_this()
  // can wrap a valid weak_ptr (in constructor, not guaranteed)

  pluginManager->main(shared_from_this(), &pluginPanels);

  std::ifstream cams("cams.json");
  if (cams) {
    JSON j;
    cams >> j;
    cameraStack = std::make_shared<std::vector<CameraState>>(j.get<std::vector<CameraState>>());
  }

  if (parseCommandLine()) {
    refreshRenderer();
    refreshScene(true);
    mainWindow->mainLoop();
  }
}

void GUIContext::updateCamera()
{
  frame->currentAccum = 0;
  auto camera = frame->child("camera").nodeAs<sg::Camera>();

  if (cameraIdx) {
    // switch to index specific scene camera
    auto &newCamera = g_sceneCameras.at_index(cameraIdx);
    g_selectedSceneCamera = newCamera.second;
    frame->remove("camera");
    frame->add(g_selectedSceneCamera);
    // update camera pointer
    camera = frame->child("camera").nodeAs<sg::Camera>();
    if (g_selectedSceneCamera->hasChild("aspect"))
      lockAspectRatio =
          g_selectedSceneCamera->child("aspect").valueAs<float>();
    mainWindow->reshape(windowSize); // resets aspect
    cameraIdx = 0; // reset global-context cameraIndex
    mainWindow->arcballCamera->updateCameraToWorld(affine3f{one}, one);
    cameraView = nullptr; // only used for arcball/default
  } else if (cameraView && *cameraView != affine3f{one}) {
    // use camera settings from scene camera
    if (cameraSettingsIdx) {
      auto settingsCamera = g_sceneCameras.at_index(cameraSettingsIdx).second;
      for (auto &c : settingsCamera->children()) {
        if (c.first == "cameraId") {
          camera->createChild("cameraSettingsId", "int", c.second->value());
          camera->child("cameraSettingsId").setSGNoUI();
          camera->child("cameraSettingsId").setSGOnly();
        } else if (c.first != "uniqueCameraName") {
          if (camera->hasChild(c.first))
            camera->child(c.first) = c.second->value();
          else {
            camera->createChild(
                c.first, c.second->subType(), c.second->value());
            if (settingsCamera->child(c.first).sgOnly())
              camera->child(c.first).setSGOnly();
          }
        }
      }
      if (settingsCamera->hasChild("aspect"))
        lockAspectRatio =
            settingsCamera->child("aspect").valueAs<float>();
      mainWindow->reshape(windowSize); // resets aspect
    }

    auto worldToCamera = rcp(*cameraView);
    LinearSpace3f R, S;
    ospray::sg::getRSComponent(worldToCamera, R, S);
    auto rotation = ospray::sg::getRotationQuaternion(R);

    mainWindow->arcballCamera->updateCameraToWorld(*cameraView, rotation);
    cameraView = nullptr;
    mainWindow->centerOnEyePos();
  }

  camera->child("position").setValue(mainWindow->arcballCamera->eyePos());
  camera->child("direction").setValue(mainWindow->arcballCamera->lookDir());
  camera->child("up").setValue(mainWindow->arcballCamera->upDir());

  if (camera->hasChild("focusDistance")
      && !camera->child("cameraId").valueAs<int>()) {
    float focusDistance =
        rkcommon::math::length(camera->child("lookAt").valueAs<vec3f>()
            - mainWindow->arcballCamera->eyePos());
    if (camera->child("adjustAperture").valueAs<bool>()) {
      float oldFocusDistance = camera->child("focusDistance").valueAs<float>();
      if (!(isinf(oldFocusDistance) || isinf(focusDistance))) {
        float apertureRadius = camera->child("apertureRadius").valueAs<float>();
        camera->child("apertureRadius")
            .setValue(apertureRadius * focusDistance / oldFocusDistance);
      }
    }
    camera->child("focusDistance").setValue(focusDistance);
  }
}

void GUIContext::changeToDefaultCamera()
{
  auto defaultCamera = g_sceneCameras["default"];
  auto sgSceneCamera = frame->child("camera").nodeAs<sg::Camera>();

  for (auto &c : sgSceneCamera->children()) {
    if (c.first == "cameraId") {
      defaultCamera->createChild("cameraSettingsId", "int", c.second->value());
      defaultCamera->child("cameraSettingsId").setSGNoUI();
      defaultCamera->child("cameraSettingsId").setSGOnly();
    } else if (c.first != "uniqueCameraName") {
      if (defaultCamera->hasChild(c.first))
        defaultCamera->child(c.first) = c.second->value();
      else {
        defaultCamera->createChild(
            c.first, c.second->subType(), c.second->value());
        if (sgSceneCamera->child(c.first).sgOnly())
          defaultCamera->child(c.first).setSGOnly();
      }
    }
  }

  frame->remove("camera");
  frame->add(defaultCamera);
  frame->commit();

  auto worldToCamera = rcp(sgSceneCamera->cameraToWorld);
  LinearSpace3f R, S;
  ospray::sg::getRSComponent(worldToCamera, R, S);
  auto rotation = ospray::sg::getRotationQuaternion(R);

  mainWindow->arcballCamera->updateCameraToWorld(
      sgSceneCamera->cameraToWorld, rotation);
  mainWindow->centerOnEyePos();
  updateCamera(); // to reflect new default camera properties in GUI
}

void GUIContext::refreshRenderer()
{
  // Change renderer if current type doesn't match requested
  auto currentType = frame->childAs<sg::Renderer>("renderer")
                         .child("type")
                         .valueAs<std::string>();
  if (currentType != optRendererTypeStr)
    frame->createChild("renderer", "renderer_" + optRendererTypeStr);

  auto &r = frame->childAs<sg::Renderer>("renderer");
  r["pixelFilter"] = (int)optPF;
  r["backgroundColor"] = optBackGroundColor;
  r["pixelSamples"] = optSPP;
  r["varianceThreshold"] = optVariance;
  if (r.hasChild("maxContribution") && maxContribution < (float)math::inf)
    r["maxContribution"] = maxContribution;

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
  } else {
    // Node removal requires waiting on previous frame completion
    frame->cancelFrame();
    frame->waitOnFrame();
    r.remove("map_backplate");
    r.handle().removeParam("map_backplate");
  }
}

void GUIContext::saveRendererParams()
{
  auto &r = frame->childAs<sg::Renderer>("renderer");

  optRendererTypeStr = r["type"].valueAs<std::string>();
  optPF = (OSPPixelFilterTypes)r["pixelFilter"].valueAs<int>();
  optBackGroundColor = r["backgroundColor"].valueAs<sg::rgba>();
  optSPP = r["pixelSamples"].valueAs<int>();
  optVariance = r["varianceThreshold"].valueAs<float>();
  if (r.hasChild("maxContribution"))
    maxContribution = r["maxContribution"].valueAs<float>();
}

void GUIContext::refreshScene(bool resetCam)
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
        scene + "_generator",
        "generator_" + scene);
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
    mainWindow->arcballCamera.reset(new ArcballCamera(worldBounds, windowSize));
  }
  
  updateCamera();
  auto &fb = frame->childAs<sg::FrameBuffer>("framebuffer");
  fb.resetAccumulation();
}

void GUIContext::addToCommandLine(std::shared_ptr<CLI::App> app) {}

bool GUIContext::parseCommandLine()
{
  int ac = studioCommon.argc;
  const char **av = studioCommon.argv;

  std::shared_ptr<CLI::App> app =
      std::make_shared<CLI::App>("OSPRay Studio GUI");
  StudioContext::addToCommandLine(app);
  GUIContext::addToCommandLine(app);
  try {
    app->parse(ac, av);
  } catch (const CLI::ParseError &e) {
    exit(app->exit(e));
  }

  // XXX: changing windowSize here messes causes some display scaling issues
  // because it desyncs window and framebuffer size with any scaling
  if (optResolution.x != 0) {
    windowSize = optResolution;
    mainWindow->reshape(windowSize);
  }
  return true;
}

// Importer for all known file types (geometry and models)
void GUIContext::importFiles(sg::NodePtr world)
{
  std::shared_ptr<CameraMap> cameras{nullptr};
  if (!sgFileCameras) {
    cameras = std::make_shared<CameraMap>();
    // populate cameras map with default camera
    auto mainCamera = frame->child("camera").nodeAs<sg::Camera>();
    cameras->operator[](
        mainCamera->child("uniqueCameraName").valueAs<std::string>()) =
        mainCamera;
  }

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
          if (sgFileCameras) {
            importer->importCameras = false;
            importer->setCameraList(sgFileCameras);
          } else if (cameras) {
            importer->importCameras = true;
            importer->setCameraList(cameras);
          }
          importer->setLightsManager(lightsManager);
          importer->setArguments(studioCommon.argc, (char **)studioCommon.argv);
          importer->setScheduler(scheduler);
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

    if (!optDoAsyncTasking) {
      for (;;) {
        size_t numTasksExecuted = 0;

        numTasksExecuted += scheduler->background()->executeAllTasksSync();
        numTasksExecuted += scheduler->ospray()->executeAllTasksSync();
        numTasksExecuted += scheduler->studio()->executeAllTasksSync();

        if (numTasksExecuted == 0) {
          break;
        }
      }
    }
  }
  filesToImport.clear();

  // Initializes time range for newly imported models
  mainWindow->animationWidget->init();

  if (sgFileCameras)
    g_sceneCameras = *sgFileCameras;
  else if (cameras)
    g_sceneCameras = *cameras;
}

void GUIContext::saveCurrentFrame()
{
  int filenum = 0;
  char filename[64];
  const char *ext = optImageFormat.c_str();

  // Find an unused filename to ensure we don't overwrite and existing file
  do
    std::snprintf(
        filename, 64, "%s.%04d.%s", optImageName.c_str(), filenum++, ext);
  while (std::ifstream(filename).good());

  int screenshotFlags = optSaveLayersSeparately << 3 | optSaveNormal << 2
      | optSaveDepth << 1 | optSaveAlbedo;

  auto &fb = frame->childAs<sg::FrameBuffer>("framebuffer");
  auto fbFloatFormat = fb["floatFormat"].valueAs<bool>();
  if (screenshotFlags > 0 && !fbFloatFormat)
    std::cout
        << " *** Cannot save additional information without changing FB format to float ***"
        << std::endl;
  frame->saveFrame(std::string(filename), screenshotFlags);
}

void GUIContext::printUtilNode(const std::string &utilNode)
{
  if (utilNode == "camera")
    frame->child("camera").traverse<sg::PrintNodes>();
  else if (utilNode == "frame")
    frame->traverse<sg::PrintNodes>();
  else if (utilNode == "baseMaterialRegistry")
    baseMaterialRegistry->traverse<sg::PrintNodes>();
  else if (utilNode == "lightsManager")
    lightsManager->traverse<sg::PrintNodes>();
}

bool GUIContext::resHasHit(float &x, float &y, vec3f &worldPosition)
{
  ospray::cpp::PickResult res;
  auto &fb = frame->childAs<sg::FrameBuffer>("framebuffer");
  auto &r = frame->childAs<sg::Renderer>("renderer");
  auto &c = frame->childAs<sg::Camera>("camera");
  auto &w = frame->childAs<sg::World>("world");
  res = fb.handle().pick(r, c, w, x, y);
  x = clamp(x / windowSize.x, 0.f, 1.f);
  y = 1.f - clamp(y / windowSize.y, 0.f, 1.f);
  worldPosition = res.worldPosition;
  if (res.hasHit) {
    c["lookAt"] = vec3f(worldPosition);
    updateCamera();
  }
  return res.hasHit;
}

void GUIContext::getWindowTitle(std::stringstream &windowTitle)
{
  windowTitle << "OSPRay Studio: ";

  auto &fb = frame->childAs<sg::FrameBuffer>("framebuffer");
  auto &v = frame->childAs<sg::Renderer>("renderer")["varianceThreshold"];
  auto varianceThreshold = v.valueAs<float>();

  if (frame->pauseRendering) {
    windowTitle << "rendering paused";
  } else if (frame->accumLimitReached()) {
    windowTitle << "accumulation limit reached";
  } else if (fb.variance() < varianceThreshold) {
    windowTitle << "variance threshold reached";
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
}

vec2i &GUIContext::getFBSize()
{
  auto &frameBuffer = frame->childAs<sg::FrameBuffer>("framebuffer");
  return frameBuffer.child("size").valueAs<vec2i>();
}

void GUIContext::updateFovy(const float &sensitivity, const float &scrollY)
{
  auto &camera = frame->child("camera");
  if (camera.hasChild("fovy")) {
    auto fovy = camera["fovy"].valueAs<float>();
    fovy = std::min(180.f, std::max(0.f, fovy + scrollY * sensitivity));
    camera["fovy"] = fovy;
    updateCamera();
  }
}

void GUIContext::useSceneCamera()
{
  // use scene camera while playing animation
  if (frame->child("camera").child("uniqueCameraName").valueAs<std::string>()
          == "default"
      && g_selectedSceneCamera) {
    frame->remove("camera");
    frame->add(g_selectedSceneCamera);
  }
}

void GUIContext::saveSGScene()
{
  std::ofstream dump("studio_scene.sg");
  auto &currentCamera = frame->child("camera");
  JSON camera = {{"cameraIdx", currentCamera.child("cameraId").valueAs<int>()},
      {"cameraToWorld", mainWindow->arcballCamera->getTransform()}};
  if (currentCamera.hasChild("cameraSettingsId"))
    camera["cameraSettingsIdx"] =
        currentCamera.child("cameraSettingsId").valueAs<int>();
  JSON animation;
  animation = {{"time", animationManager->getTime()},
      {"shutter", animationManager->getShutter()}};
  JSON j = {{"world", frame->child("world")},
      {"camera", camera},
      {"lightsManager", *lightsManager},
      {"materialRegistry", *baseMaterialRegistry},
      {"animation", animation}};
  dump << j.dump();
}

void GUIContext::saveNodesJson(const std::string nodeTypeStr)
{
  if (nodeTypeStr == "baseMaterialRegistry") {
    std::ofstream materials("studio_materials.sg");
    JSON j = {{"materialRegistry", *baseMaterialRegistry}};
    materials << j.dump();
  }
  if (nodeTypeStr == "lightsManager") {
    std::ofstream lights("studio_lights.sg");
    JSON j = {{"lightsManager", *lightsManager}};
    lights << j.dump();
  }
  if (nodeTypeStr == "Camera") {
    std::ofstream camera("studio_camera.sg");
    JSON j = {{"camera", mainWindow->arcballCamera->getState()}};
    camera << j.dump();
  }
}

void GUIContext::selectBuffer(int whichBuffer)
{
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
}

void GUIContext::addCameraState()
{
  if (cameraStack->empty()) {
    cameraStack->push_back(mainWindow->arcballCamera->getState());
    g_camSelectedStackIndex = 0;
  } else {
    cameraStack->insert(cameraStack->begin() + g_camSelectedStackIndex + 1,
        mainWindow->arcballCamera->getState());
    g_camSelectedStackIndex++;
  }
}

void GUIContext::removeCameraState()
{
  // remove the selected camera state
  cameraStack->erase(cameraStack->begin() + g_camSelectedStackIndex);
  g_camSelectedStackIndex = std::max(0, g_camSelectedStackIndex - 1);
}

void GUIContext::viewCameraPath(bool showCameraPath)
{
  if (!showCameraPath) {
    frame->child("world").remove("cameraPath_xfm");
    refreshScene(false);
  } else {
    auto pathXfm = sg::createNode("cameraPath_xfm", "transform");

    const auto &worldBounds = frame->child("world").bounds();
    float pathRad = 0.0075f * reduce_min(worldBounds.size());
    std::vector<CameraState> cameraPath =
        buildPath(*cameraStack, g_camPathSpeed * 0.01f);
    std::vector<vec4f> pathVertices; // position and radius
    for (const auto &state : cameraPath)
      pathVertices.emplace_back(state.position(), pathRad);
    pathVertices.emplace_back(cameraStack->back().position(), pathRad);

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
    for (int i = 0; i < cameraStack->size(); i++) {
      capVertexes.push_back(cameraStack->at(i).position());
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

void GUIContext::removeLight(int whichLight)
{
  if (whichLight != -1) {
    // Node removal requires waiting on previous frame completion
    frame->cancelFrame();
    frame->waitOnFrame();
    auto &lights = lightsManager->children();
    lightsManager->removeLight(lights.at_index(whichLight).first);
    whichLight = std::max(0, whichLight - 1);
  }
}

void GUIContext::selectCamera(size_t whichCamera)
{
  if (whichCamera > -1 && whichCamera < (int)g_sceneCameras.size()) {
    auto &newCamera = g_sceneCameras.at_index(whichCamera);
    g_selectedSceneCamera = newCamera.second;
    auto hasParents = g_selectedSceneCamera->parents().size();
    frame->remove("camera");
    frame->add(g_selectedSceneCamera);

    if (g_selectedSceneCamera->hasChild("aspect"))
      lockAspectRatio =
          g_selectedSceneCamera->child("aspect").valueAs<float>();
    mainWindow->reshape(windowSize); // resets aspect
    if (!hasParents)
      updateCamera();
  }
}

void GUIContext::createNewCamera(const std::string newType)
{
  frame->createChildAs<sg::Camera>("camera", newType);
  mainWindow->reshape(windowSize); // resets aspect
  updateCamera();
}

void GUIContext::createIsoSurface(
    int currentVolume, std::vector<ospray::sg::NodePtr> &volumes)
{
  auto selected = volumes.at(currentVolume);

  auto &world = frame->childAs<sg::World>("world");

  auto count = 1;
  auto surfName = selected->name() + "_surf";
  while (world.hasChild(surfName + std::to_string(count) + "_xfm"))
    count++;
  surfName += std::to_string(count);

  auto isoXfm = sg::createNode(surfName + "_xfm", "transform", affine3f{one});

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
  mat->child("ns").setMinMax(2.f, 10000.f);
  mat->child("d").setMinMax(0.f, 1.f);

  baseMaterialRegistry->add(mat);
  isoGeom->createChildData("material", mID);
  isoGeom->child("material").setSGOnly();

  auto &handle = isoGeom->valueAs<cpp::Geometry>();
  handle.setParam("volume", selected->valueAs<cpp::Volume>());

  isoXfm->add(isoGeom);

  world.add(isoXfm);
}

void GUIContext::clearScene()
{
  // Cancel any in-progress frame
  frame->cancelFrame();
  frame->waitOnFrame();
  frame->remove("world");
  lightsManager->clear();
  animationManager->getAnimations().clear();
  animationManager->setTimeRange(range1f(rkcommon::math::empty));
  mainWindow->animationWidget->update();

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
}

void GUIContext::renderTexturedQuad(vec2f &border)
{
  // render textured quad with OSPRay frame buffer contents
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
}

void GUIContext::setLockUpDir(const vec3f &lockUpDir)
{
  mainWindow->arcballCamera->setLockUpDir(lockUpDir);
}

void GUIContext::setUpDir(const vec3f &upDir)
{
  mainWindow->arcballCamera->setUpDir(upDir);
}

void GUIContext::animationSetShowUI()
{
  mainWindow->animationWidget->setShowUI();
}

void GUIContext::quitNextFrame() {
  mainWindow->g_quitNextFrame = true;
}

void GUIContext::setCameraSnapshot(size_t snapshot)
{
  if (snapshot < cameraStack->size()) {
    const CameraState &cs = cameraStack->at(snapshot);
    arcballCamera->setState(cs);
    updateCamera();
  }
}

void GUIContext::selectCamStackIndex(size_t i)
{
  g_camSelectedStackIndex = i;
  arcballCamera->setState(cameraStack->at(i));
  updateCamera();
}

