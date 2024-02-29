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
using namespace ospray::sg;

MainWindow *GUIContext::mainWindow = nullptr;

// GUIContext definitions ///////////////////////////////////////////////

GUIContext::GUIContext(StudioCommon &_common)
    : StudioContext(_common, StudioMode::GUI)
{
  pluginManager = std::make_shared<PluginManager>();
  optSPP = 1; // Default SamplesPerPixel in interactive mode is one.
  if (frame->hasChild("framebuffer"))
    framebuffer = frame->child("framebuffer").nodeAs<FrameBuffer>();
  defaultSize = _common.defaultSize;

  // Define "default" camera in cameras list
  g_sceneCameras["default"] = frame->child("camera").nodeAs<Camera>();
}

GUIContext::~GUIContext()
{
  pluginManager->removeAllPlugins();
  g_sceneCameras.clear();
  clearAssets();
}

void GUIContext::start()
{
  std::cerr << "GUI mode\n";

  auto currentUtil = std::shared_ptr<GUIContext>(this);
  if (!mainWindow) {
    mainWindow = new MainWindow(defaultSize, currentUtil);
    mainWindow->initGLFW();
    StudioContext::setMainWindow((void *)mainWindow);
  }

  // load plugins //
  for (auto &p : studioCommon.pluginsToLoad)
    pluginManager->loadPlugin(p);

  // create panels //
  // doing this outside constructor to ensure shared_from_this()
  // can wrap a valid weak_ptr (in constructor, not guaranteed)
  pluginManager->main(shared_from_this(), &pluginPanels);

  // set camera correctly to Id set externally via JSON or plugins:
  frame->child("camera").child("cameraId").setValue(whichCamera);
  cameraIdx = whichCamera;

  if (parseCommandLine()) {
    loadCamJson();
    // If command line options are set, enable denoiser
    if (studioCommon.denoiserAvailable && optDenoiser) {
      frame->denoiseFB = true;
      frame->denoiseFBFinalFrame = optDenoiseFinalFrame;
      framebuffer->child("floatFormat") = true;
      framebuffer->commit();
    }
    refreshRenderer();
    refreshScene(true);
    mainWindow->mainLoop();
    if (optSaveImageOnGUIExit)
      saveCurrentFrame();
  }
}

void GUIContext::loadCamJson()
{
  std::ifstream cams(optCamJsonName);
  if (cams) {
    std::cout << "Load cameras for keyframe/snapshots from " << optCamJsonName
              << std::endl;

    JSON j;
    cams >> j;
    mainWindow->cameraStack->setValues(j.get<std::vector<CameraState>>());
  }
}

void GUIContext::updateCamera()
{
  frame->currentAccum = 0;
  auto camera = frame->child("camera").nodeAs<Camera>();

  if (cameraIdx) {
    // switch to index specific scene camera
    auto &newCamera = g_sceneCameras.at_index(cameraIdx);
    g_selectedSceneCamera = newCamera.second;
    frame->remove("camera");
    frame->add(g_selectedSceneCamera);
    // update camera pointer
    camera = frame->child("camera").nodeAs<Camera>();
    if (g_selectedSceneCamera->hasChild("aspect"))
      lockAspectRatio =
          g_selectedSceneCamera->child("aspect").valueAs<float>();
    mainWindow->reshape(); // resets aspect
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
      mainWindow->reshape(); // resets aspect
    }

    auto worldToCamera = rcp(*cameraView);
    LinearSpace3f R, S;
    getRSComponent(worldToCamera, R, S);
    auto rotation = getRotationQuaternion(R);

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
  if (frame->child("camera").child("uniqueCameraName").valueAs<std::string>()
      == "default")
    return;

  auto defaultCamera = g_sceneCameras["default"];
  auto sgSceneCamera = frame->child("camera").nodeAs<Camera>();

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

  cameraIdx = 0; // reset global-context cameraIndex
  frame->remove("camera");
  frame->add(defaultCamera);
  frame->commit();

  auto worldToCamera = rcp(sgSceneCamera->cameraToWorld);
  LinearSpace3f R, S;
  getRSComponent(worldToCamera, R, S);
  auto rotation = getRotationQuaternion(R);

  mainWindow->arcballCamera->updateCameraToWorld(
      sgSceneCamera->cameraToWorld, rotation);
  mainWindow->centerOnEyePos();
  updateCamera(); // to reflect new default camera properties in GUI
}

void GUIContext::refreshRenderer()
{
  // Change renderer if current type doesn't match requested
  auto currentType = frame->childAs<Renderer>("renderer")
                         .child("type")
                         .valueAs<std::string>();
  if (currentType != optRendererTypeStr)
    frame->createChild("renderer", "renderer_" + optRendererTypeStr);

  auto &r = frame->childAs<Renderer>("renderer");
  r["pixelFilter"] = optPF;
  r["backgroundColor"] = optBackGroundColor;
  r["pixelSamples"] = optSPP;
  r["varianceThreshold"] = optVariance;
  if (r.hasChild("maxContribution") && maxContribution < (float)math::inf)
    r["maxContribution"] = maxContribution;

  // Re-add the backplate on renderer change
  r["backplate_filename"] = backPlateTexture.str();
}

void GUIContext::saveRendererParams()
{
  auto &r = frame->childAs<Renderer>("renderer");

  optRendererTypeStr = r["type"].valueAs<std::string>();
  optPF = r["pixelFilter"].valueAs<OSPPixelFilterType>();
  optBackGroundColor = r["backgroundColor"].valueAs<rgba>();
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
  auto world = frame->hasChild("world") ? frame->childNodeAs<Node>("world")
                                        : createNode("world", "world");
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
    auto &gen = world->createChildAs<Generator>(
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
    // Switch back to default-camera before modifying any parameters
    changeToDefaultCamera();
    mainWindow->resetArcball();
  }
  
  updateCamera();
  auto &fb = frame->childAs<FrameBuffer>("framebuffer");
  fb.resetAccumulation();
}

void GUIContext::addToCommandLine(std::shared_ptr<CLI::App> app) {
  app->add_flag(
    "--saveImageOnExit",
    optSaveImageOnGUIExit,
    "Save final image when exiting GUI mode"
  );
  app->add_option(
    "--scene",
    scene,
    "Sets the opening scene name"
  );
}

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
    app->exit(e);
    return false;
  }

  // XXX: changing windowSize here messes causes some display scaling issues
  // because it desyncs window and framebuffer size with any scaling
  if (optResolution.x != 0) {
    defaultSize = optResolution;
    // since parseCommandLine happens after MainWindow object creation update the windowSize of that class
    mainWindow->windowSize = defaultSize;
    mainWindow->reshape(true);
  }
  return true;
}

// Importer for all known file types (geometry and models)
void GUIContext::importFiles(NodePtr world)
{
  std::shared_ptr<CameraMap> cameras{nullptr};
  if (!sgFileCameras) {
    cameras = std::make_shared<CameraMap>();
    // populate cameras map with default camera
    auto mainCamera = frame->child("camera").nodeAs<Camera>();
    cameras->operator[](
        mainCamera->child("uniqueCameraName").valueAs<std::string>()) =
        mainCamera;
  }

  for (auto file : filesToImport) {
    try {
      rkcommon::FileName fileName(file);

      // XXX: handling loading a scene here for now, it requires the entire
      // context.
      if (fileName.ext() == "sg") {
        importScene(shared_from_this(), fileName);
        sgScene = true;
      } else {
        std::cout << "Importing: " << file << std::endl;

        auto importer = getImporter(world, file, optReloadAssets);
        if (importer) {
          auto vp = importer->getVolumeParams();
          if (volumeParams->children().size() > 0 && vp) {
            std::cout << "Using command-line volume parameters ..." << std::endl;
            for (auto &c : volumeParams->children()) {
              vp->remove(c.first);
              vp->add(c.second);
            }
          } else 
            importer->setVolumeParams(volumeParams);

          importer->verboseImport = optVerboseImporter;
          importer->pointSize = pointSize;
          importer->setFb(frame->childAs<FrameBuffer>("framebuffer"));
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
                InstanceConfiguration::DYNAMIC);
          else if (optInstanceConfig == "compact")
            importer->setInstanceConfiguration(
                InstanceConfiguration::COMPACT);
          else if (optInstanceConfig == "robust")
            importer->setInstanceConfiguration(
                InstanceConfiguration::ROBUST);

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

  const auto &newCameras = sgFileCameras ? *sgFileCameras : *cameras;
  if (!newCameras.empty()) {
    for (const auto &camera : newCameras)
      if (camera.second)
        g_sceneCameras[camera.first] = camera.second;
  }
}

void GUIContext::saveCurrentFrame()
{
  std::string filename;
  char filenumber[16];
  int filenum = 0;

  // Find an unused filename to ensure we don't overwrite an existing file
  // XXX refactor to allow forceRewrite option, as in batch mode
  do {
    std::snprintf(filenumber, 16, ".%05d.", filenum++);
    filename = optImageName + filenumber + optImageFormat;
  } while (std::ifstream(filename).good());

  int screenshotFlags = optSaveLayersSeparately << 3 | optSaveNormal << 2
      | optSaveDepth << 1 | optSaveAlbedo;

  auto &fb = frame->childAs<FrameBuffer>("framebuffer");
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
    frame->child("camera").traverse<PrintNodes>();
  else if (utilNode == "frame")
    frame->traverse<PrintNodes>();
  else if (utilNode == "baseMaterialRegistry")
    baseMaterialRegistry->traverse<PrintNodes>();
  else if (utilNode == "lightsManager")
    lightsManager->traverse<PrintNodes>();
}

bool GUIContext::resHasHit(float &x, float &y, vec3f &worldPosition)
{
  ospray::cpp::PickResult res;
  auto &fb = frame->childAs<FrameBuffer>("framebuffer");
  auto &r = frame->childAs<Renderer>("renderer");
  auto &c = frame->childAs<Camera>("camera");
  auto &w = frame->childAs<World>("world");
  res = fb.handle().pick(r, c, w, x, y);
  worldPosition = res.worldPosition;
  if (res.hasHit)
    c["lookAt"] = vec3f(worldPosition);
  return res.hasHit;
}

void GUIContext::getWindowTitle(std::stringstream &windowTitle)
{
  windowTitle << "OSPRay Studio: ";

  auto &fb = frame->childAs<FrameBuffer>("framebuffer");
  auto &v = frame->childAs<Renderer>("renderer")["varianceThreshold"];
  auto varianceThreshold = v.valueAs<float>();

  if (frame->pauseRendering) {
    windowTitle << "rendering paused";
  } else if (frame->accumLimitReached()) {
    windowTitle << "accumulation limit reached";
  } else if (fb.variance() < varianceThreshold) {
    windowTitle << "variance threshold reached";
  } else {
    auto latestFPS = mainWindow->latestFPS;
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
  auto &frameBuffer = frame->childAs<FrameBuffer>("framebuffer");
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
  if (nodeTypeStr == "camera") {
    std::ofstream camera("studio_camera.sg");
    JSON j = {{"camera", mainWindow->arcballCamera->getState()}};
    camera << j.dump();
  }
}

void GUIContext::selectBuffer(OSPFrameBufferChannel whichBuffer, bool invert)
{
  auto &framebuffer = frame->childAs<FrameBuffer>("framebuffer");

  optDisplayBuffer = whichBuffer;
  optDisplayBufferInvert = invert && framebuffer.isFloatFormat();

  // Only enabled if they exist
  if (!framebuffer.hasDepthChannel())
    optDisplayBuffer &= ~OSP_FB_DEPTH;
  if (!framebuffer.hasAccumChannel())
    optDisplayBuffer &= ~OSP_FB_ACCUM;
  if (!framebuffer.hasVarianceChannel())
    optDisplayBuffer &= ~OSP_FB_VARIANCE;
  if (!framebuffer.hasNormalChannel())
    optDisplayBuffer &= ~OSP_FB_NORMAL;
  if (!framebuffer.hasAlbedoChannel())
    optDisplayBuffer &= ~OSP_FB_ALBEDO;
  if (!framebuffer.hasPrimitiveIDChannel())
    optDisplayBuffer &= ~OSP_FB_ID_PRIMITIVE;
  if (!framebuffer.hasObjectIDChannel())
    optDisplayBuffer &= ~OSP_FB_ID_OBJECT;
  if (!framebuffer.hasInstanceIDChannel())
    optDisplayBuffer &= ~OSP_FB_ID_INSTANCE;

  // If nothing else is left enabled, choose color
  if (!optDisplayBuffer)
    optDisplayBuffer = OSP_FB_COLOR;
}

void GUIContext::selectCamera()
{
  if (whichCamera < (int)g_sceneCameras.size()) {
    auto &newCamera = g_sceneCameras.at_index(whichCamera);
    g_selectedSceneCamera = newCamera.second;
    g_selectedSceneCamera->child("cameraId").setValue(whichCamera);
    auto hasParents = g_selectedSceneCamera->parents().size();
    frame->remove("camera");
    frame->add(g_selectedSceneCamera);

    if (g_selectedSceneCamera->hasChild("aspect"))
      lockAspectRatio =
          g_selectedSceneCamera->child("aspect").valueAs<float>();
    mainWindow->reshape(); // resets aspect
    if (!hasParents)
      updateCamera();
  }
}

void GUIContext::createNewCamera(const std::string newType)
{
  frame->createChildAs<Camera>("camera", newType);
  mainWindow->reshape(); // resets aspect
  updateCamera();
}

void GUIContext::createIsoSurface(
    int currentVolume, std::vector<NodePtr> &volumes)
{
  auto selected = volumes.at(currentVolume);

  auto &world = frame->childAs<World>("world");

  auto count = 1;
  auto surfName = selected->name() + "_surf";
  while (world.hasChild(surfName + std::to_string(count) + "_xfm"))
    count++;
  surfName += std::to_string(count);

  auto isoXfm = createNode(surfName + "_xfm", "transform", affine3f{one});

  auto valueRange = selected->child("value").valueAs<range1f>();

  auto isoGeom = createNode(surfName, "geometry_isosurfaces");
  isoGeom->createChild("value", "range1f", valueRange);
  isoGeom->child("value").setSGOnly();
  isoGeom->createChild("isovalue", "float", valueRange.center());
  isoGeom->child("isovalue").setMinMax(valueRange.lower, valueRange.upper);

  uint32_t materialID = baseMaterialRegistry->baseMaterialOffSet();
  const std::vector<uint32_t> mID = {materialID};
  auto mat = createNode(surfName, "obj");

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
  clearAssets();

  // Recreate MaterialRegistry, clearing old registry and all materials
  // Then, add the new one to the frame and set the renderer type
  baseMaterialRegistry = createNodeAs<MaterialRegistry>(
      "baseMaterialRegistry", "materialRegistry");
  frame->add(baseMaterialRegistry);
  baseMaterialRegistry->updateRendererType();
  scene = "";
  refreshScene(true);
}
