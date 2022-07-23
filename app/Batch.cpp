// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Batch.h"
// ospray_sg
#include "sg/Frame.h"
#include "sg/fb/FrameBuffer.h"
#include "sg/renderer/MaterialRegistry.h"
#include "sg/visitors/Commit.h"
#include "sg/visitors/PrintNodes.h"
#include "sg/camera/Camera.h"
#include "sg/scene/volume/Volume.h"
#include "sg/Mpi.h"

// rkcommon
#include "rkcommon/utility/SaveImage.h"
// json
#include "sg/JSONDefs.h"
#include "PluginManager.h"

// CLI
#include <CLI11.hpp>

static bool resetFileId = false;

BatchContext::BatchContext(StudioCommon &_common)
    : StudioContext(_common, StudioMode::BATCH)
{
  frame->child("scaleNav").setValue(1.f);
  pluginManager = std::make_shared<PluginManager>();

  // Default saved image baseName (cmdline --image to override)
  optImageName = "ospBatch";
}

void BatchContext::start()
{
  std::cerr << "Batch mode\n";

  // load plugins
  for (auto &p : studioCommon.pluginsToLoad)
    pluginManager->loadPlugin(p);

  // read from cams.json
  std::ifstream cams("cams.json");
  if (cams) {
    JSON j;
    cams >> j;
    for (auto &cs : j) {
      if (cs.find("cameraToWorld") != cs.end())
        cameraStack.push_back(cs.at("cameraToWorld"));
    }
  }

  if (parseCommandLine()) {
    std::cout << "...importing files!" << std::endl;
    refreshRenderer();
    refreshScene(true);

    if (cameras->size())
      std::cout << "List of imported scene cameras:\n";
    for (int c = 1; c <= cameras->size(); ++c) {
      std::cout
          << c << ": "
          << cameras->at_index(c - 1).second->child("uniqueCameraName").valueAs<std::string>()
          << std::endl;
    }

    optCameraRange.upper = std::min(optCameraRange.upper, (int)cameras->size());
    for (int cameraIdx = optCameraRange.lower; cameraIdx <= optCameraRange.upper;
         ++cameraIdx) {
      resetFileId = true;
      refreshCamera(cameraIdx);

      // if a camera stack is present loop over every entry of camera stack for
      // every camera
      if (cameraStack.size())
        for (auto &c : cameraStack) {
          cameraView = std::make_shared<affine3f>(c);
          if (!sgScene)
            updateCamera();
          render();
          if (fps) {
            std::cout << "..rendering animation!" << std::endl;
            renderAnimation();
          } else
            renderFrame();
        }
      else {
        if (!sgScene)
          updateCamera();
        render();
        if (fps) {
          std::cout << "..rendering animation!" << std::endl;
          renderAnimation();
        } else
          renderFrame();
      }
    }

    std::cout << "...finished!" << std::endl;
    sg::clearAssets();
  }
}

void BatchContext::addToCommandLine(std::shared_ptr<CLI::App> app) {
  app->add_option(
    "--cameraType",
    optCameraTypeStr,
    "Set the camera type"
  )->check(CLI::IsMember({"perspective", "orthographic", "panoramic"}));
  app->add_option(
    "--position",
    [&](const std::vector<std::string> val) {
      pos = vec3f(std::stof(val[0]), std::stof(val[1]), std::stof(val[2]));
      cmdlCam = true;
      return true;
    },
    "Set the camera position"
  )->expected(3);
  app->add_option(
    "--view",
    [&](const std::vector<std::string> val) {
      gaze = vec3f(std::stof(val[0]), std::stof(val[1]), std::stof(val[2]));
      cmdlCam = true;
      return true;
    },
    "Set the camera view vector"
  )->expected(3);
  app->add_option(
    "--up",
    [&](const std::vector<std::string> val) {
      up = vec3f(std::stof(val[0]), std::stof(val[1]), std::stof(val[2]));
      cmdlCam = true;
      return true;
    },
    "Set the camera up vector"
  )->expected(3);
  app->add_option(
    "--interpupillaryDistance",
    optInterpupillaryDistance,
    "Set the interpupillary distance"
  )->check(CLI::PositiveNumber);
  app->add_option(
    "--stereoMode",
    optStereoMode,
    "Set the stereo mode"
  )->check(CLI::PositiveNumber);
  app->add_option(
    "--grid",
    [&](const std::vector<std::string> val) {
      optGridSize = vec3i(std::stoi(val[0]), std::stoi(val[1]), std::stoi(val[2]));
      optGridEnable = true;
      return true;
    },
    "Set the camera position"
  )->expected(3);
  app->add_flag(
    "--saveMetadata",
    saveMetaData,
    "Save metadata"
  );
  app->add_option(
    "--framesPerSecond",
    fps,
    "Set the number of frames per second"
  );
  app->add_flag(
    "--forceRewrite",
    forceRewrite,
    "Force overwriting saved files if they exist"
  );
  app->add_option(
    "--frameRange",
    [&](const std::vector<std::string> val) {
      framesRange.lower = std::max(0, std::stoi(val[0]));
      framesRange.upper = std::stoi(val[1]);
      return true;
    },
    "Set the frames range"
  )->expected(2);
  app->add_option(
    "--frameStep",
    frameStep,
    "Set the frames step when (frameRange is used)"
  )->check(CLI::PositiveNumber);
  app->add_flag(
    "--saveScene",
    saveScene,
    "Saves the SceneGraph representing the frame"
  );
}

bool BatchContext::parseCommandLine()
{
  int ac = studioCommon.argc;
  const char **av = studioCommon.argv;

  // if sgFile is being imported then disable BatchContext::addToCommandLine()
  if (ac > 1) {
    for (int i = 1; i < ac; ++i) {
      std::string arg = av[i];
      std::string argExt;
      int extStart{0};
      if (arg.length() > 3)
       extStart = arg.length() - 3;
      if (extStart)
        argExt = arg.substr(extStart, arg.length());
      if (argExt == ".sg") {
        std::cout
            << "!!! When loading a scene file (.sg), fewer command-line arguments are permitted.\n"
            << "!!!   If the application exits unexpectedly, please check below for errors,\n"
            << "!!!   remove any 'not expected' arguments and try again."
            << std::endl;
        sgScene = true;
      }
    }
  }

  std::shared_ptr<CLI::App> app = std::make_shared<CLI::App>("OSPRay Studio Batch");
  StudioContext::addToCommandLine(app);
  if (!sgScene)
    BatchContext::addToCommandLine(app);
  try {
    app->parse(ac, av);
  } catch (const CLI::ParseError &e) {
    exit(app->exit(e));
  }

  if (filesToImport.size() == 0) {
    std::cout << "No files to import " << std::endl;
    return 0;
  } else
    return 1;
}

void BatchContext::refreshRenderer()
{
  frame->createChild("renderer", "renderer_" + optRendererTypeStr);
  auto &r = frame->childAs<sg::Renderer>("renderer");

  r["pixelFilter"] = (int)optPF;
  r["backgroundColor"] = optBackGroundColor;
  r["pixelSamples"] = optSPP;
  r["varianceThreshold"] = optVariance;
  if (r.hasChild("maxContribution") && maxContribution < (float)math::inf)
    r["maxContribution"].setValue(maxContribution);
}

void BatchContext::reshape()
{
  auto fSize = frame->child("windowSize").valueAs<vec2i>();

  if (lockAspectRatio) {
    // Tell OSPRay to render the largest subset of the window that satisies the
    // aspect ratio
    float aspectCorrection = lockAspectRatio * static_cast<float>(fSize.y)
        / static_cast<float>(fSize.x);
    if (aspectCorrection > 1.f) {
      fSize.y /= aspectCorrection;
    } else {
      fSize.x *= aspectCorrection;
    }
    if (frame->child("camera").hasChild("aspect"))
      frame->child("camera")["aspect"] = static_cast<float>(fSize.x) / fSize.y;
  } else if (frame->child("camera").hasChild("aspect"))
    frame->child("camera")["aspect"] = optResolution.x / (float)optResolution.y;

  frame->child("windowSize") = fSize;
  frame->currentAccum = 0;
}

void BatchContext::refreshCamera(int cameraIdx)
{
  if (cameraIdx <= cameras->size() && cameraIdx > 0) {
    std::cout << "Loading camera from index: " << std::to_string(cameraIdx)
              << std::endl;
    selectedSceneCamera = cameras->at_index(cameraIdx - 1).second;
    bool hasParent = selectedSceneCamera->parents().size() > 0;

    // TODO: remove this Hack : for some reason the accumulated transform in
    // transform node does not get updated for the BIT animation scene.
    // Attempting to make transform modified so it picks up accumulated
    // transform values made by renderScene
    if (hasParent) {
      auto cameraXfm = selectedSceneCamera->parents().front();
      if (cameraXfm->valueAs<affine3f>() == affine3f(one))
        cameraXfm->createChild("refresh", "bool");

      if (selectedSceneCamera->hasChild("aspect"))
        lockAspectRatio = selectedSceneCamera->child("aspect").valueAs<float>();

      // create unique cameraId for every camera
      if (cameraXfm->hasChild("geomId"))
        cameraId = cameraXfm->child("geomId").valueAs<std::string>();
      else
        cameraId = ".Camera_" + std::to_string(cameraIdx);
      frame->remove("camera");
      frame->add(selectedSceneCamera);
      reshape(); // resets aspect
      return;
    } else {
      std::cout << "camera not used in GLTF scene, using default camera.."
                << std::endl;
    }
  }

  std::cout << "No scene camera is selected." << std::endl;
  if (optCameraTypeStr != "perspective") {
    auto optCamera = createNode("camera", "camera_" + optCameraTypeStr);
    frame->remove("camera");
    frame->add(optCamera);
  } else
    std::cout << "using default camera..." << std::endl;

  reshape(); // resets aspect
}

void BatchContext::render()
{
  auto &frameBuffer = frame->childAs<sg::FrameBuffer>("framebuffer");
  frameBuffer["floatFormat"] = true;
  frameBuffer.commit();

  frame->child("world").createChild("materialref", "reference_to_material", 0);

  if (optGridEnable) {
    // Determine world bounds to calculate grid offsets
    frame->child("world").remove(importedModels);

    box3f bounds = getSceneBounds();
    float tx = bounds.size().x * 1.2f;
    float ty = bounds.size().y * 1.2f;
    float tz = bounds.size().z * 1.2f;

    for (auto z = 0; z < optGridSize.z; z++)
      for (auto y = 0; y < optGridSize.y; y++)
        for (auto x = 0; x < optGridSize.x; x++) {
          auto nodeName = "copy_" + std::to_string(x) + ":" + std::to_string(y)
              + ":" + std::to_string(z) + "_xfm";
          auto copy = sg::createNode(nodeName, "transform");
          copy->child("translation") = vec3f(tx * x, ty * y, tz * z);
          copy->add(importedModels);
          frame->child("world").add(copy);
        }
  }

  frame->child("navMode") = false;
}

void BatchContext::renderFrame()
{
  if (studioCommon.denoiserAvailable && optDenoiser) {
    frame->denoiseFB = true;
    frame->denoiseFBFinalFrame = true;
  }
  frame->immediatelyWait = true;

  auto &fb = frame->childAs<sg::FrameBuffer>("framebuffer");
  auto &v = frame->childAs<sg::Renderer>("renderer")["varianceThreshold"];
  auto varianceThreshold = v.valueAs<float>();
  float fbVariance{inf};

  // continue accumulation till variance threshold or accumulation limit is
  // reached
  do {
    frame->startNewFrame();
    fbVariance = fb.variance();
    std::cout << "frame " << frame->currentAccum << " ";
    std::cout << "variance " << fbVariance << std::endl;
  } while (fbVariance >= varianceThreshold && !frame->accumLimitReached());

  if (frame->denoiseFB) {
    std::cout << "denoising..." << std::endl;
    frame->startNewFrame();
  }

  static int filenum;
  if (resetFileId) {
    filenum = framesRange.lower;
    resetFileId = false;
  }

  if (!sgUsingMpi() || sgMpiRank() == 0)
  {
    std::string filename;
    char filenumber[8];
    if (!forceRewrite) {
      int filenum_ = filenum;
      do {
        std::snprintf(filenumber, 8, ".%05d.", filenum_++);
        filename = optImageName + cameraId + filenumber + optImageFormat;
      } while (std::ifstream(filename.c_str()).good());
    } else {
      std::snprintf(filenumber, 8, ".%05d.", filenum);
      filename = optImageName + cameraId + filenumber + optImageFormat;
    }
    filenum += frameStep;

    int screenshotFlags = optSaveLayersSeparately << 3 | optSaveNormal << 2
        | optSaveDepth << 1 | optSaveAlbedo;

    frame->saveFrame(filename, screenshotFlags);

    this->outputFilename = filename;

    if (saveScene)
    {
      std::ofstream dump("studio_scene.sg");
      auto &currentCamera = frame->child("camera");
      JSON camera = {
          {"cameraIdx", currentCamera.child("cameraId").valueAs<int>()},
          {"cameraToWorld", currentCamera.nodeAs<sg::Camera>()->cameraToWorld}};
      if (currentCamera.hasChild("cameraSettingsId")) {
        camera["cameraSettingsIdx"] = {"cameraSettingsIdx",
            currentCamera.child("cameraSettingsId").valueAs<int>()};
      }
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
  }
  
  pluginManager->main(shared_from_this());
}

void BatchContext::renderAnimation()
{
  float endTime = animationManager->getTimeRange().upper;
  float step = 1.f / fps;
  float time = animationManager->getTimeRange().lower;

  if (framesRange.upper >= 0) {
    endTime = std::min(animationManager->getTimeRange().upper,
        time + step * framesRange.upper + 1e-6f);
    time += step * framesRange.lower;
  }
  step *= frameStep;

  auto &cam = frame->child("camera");
  if (cam.hasChild("startTime"))
    time += cam["startTime"].valueAs<float>();
  float shutter = 0.0f;
  if (cam.hasChild("measureTime"))
    shutter = cam["measureTime"].valueAs<float>();

  while (time <= endTime) {
    animationManager->update(time, shutter);
    renderFrame();
    time += step;
  }
}

void BatchContext::refreshScene(bool resetCam)
{
  if (frameAccumLimit)
    frame->accumLimit = frameAccumLimit;
  else if (optVariance)
    frame->accumLimit = 0;
  else
    frame->accumLimit = 1;
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

  if (world->isModified()) {
    // Cancel any in-progress frame as world->render() will modify live device
    // parameters
    frame->cancelFrame();
    frame->waitOnFrame();
    world->render();
  }

  frame->add(world);

  auto &fb = frame->childAs<sg::FrameBuffer>("framebuffer");
  fb.resetAccumulation();

  frame->child("windowSize") = optResolution;
}

void BatchContext::updateCamera()
{
  frame->currentAccum = 0;
  auto camera = frame->child("camera").nodeAs<sg::Camera>();

  if (cameraIdx) {
    // switch to context global index specific scene camera
    refreshCamera(cameraIdx);
    // update camera pointer
    camera = frame->child("camera").nodeAs<sg::Camera>();
    cameraIdx = 0; // reset global-context cameraIndex
    cameraView = nullptr; // only used for arcball/default
  } else if (cameraView && *cameraView != affine3f{one}) {
    // use camera settings from scene camera if specified by global context specific 
    if (cameraSettingsIdx) {
      auto settingsCamera = cameras->at_index(cameraSettingsIdx).second;
      for (auto &c : settingsCamera->children()) {
        if (c.first == "cameraId") {
          camera->createChild("cameraSettingsId", "int", c.second->value());
          camera->child("cameraSettingsId").setSGNoUI();
          camera->child("cameraSettingsId").setSGOnly();
        } else if (c.first != "uniqueCameraName") {
          if (camera->hasChild(c.first))
            camera->child(c.first).setValue(c.second->value());
          else {
            camera->createChild(c.first, c.second->subType(), c.second->value());
            if (settingsCamera->child(c.first).sgOnly())
              camera->child(c.first).setSGOnly();
          }
        }
      }
      if (settingsCamera->hasChild("aspect"))
        lockAspectRatio = settingsCamera->child("aspect").valueAs<float>();
      reshape(); // resets aspect
    }
    affine3f cameraToWorld = *cameraView;
    camera->child("position").setValue(xfmPoint(cameraToWorld, vec3f(0, 0, 0)));
    camera->child("direction").setValue(xfmVector(cameraToWorld, vec3f(0, 0, -1)));
    camera->child("up").setValue(xfmVector(cameraToWorld, vec3f(0, 1, 0)));
    cameraView = nullptr;
  }
  // if no camera  view or scene camera is selected calculate a default view
  else if (optCameraRange.lower == 0) {
    auto worldBounds = getSceneBounds();
    auto worldDiag = length(worldBounds.size());
    auto centerTranslation = AffineSpace3f::translate(-worldBounds.center());
    auto translation = AffineSpace3f::translate(vec3f(0, 0, -worldDiag));
    auto cameraToWorld = rcp(translation * centerTranslation);

    camera->child("position").setValue(xfmPoint(cameraToWorld, vec3f(0, 0, 0)));
    camera->child("direction").setValue(xfmVector(cameraToWorld, vec3f(0, 0, -1)));
    camera->child("up").setValue(xfmVector(cameraToWorld, vec3f(0, 1, 0)));
  }

  if (cmdlCam) {
    camera->child("position").setValue(pos);
    camera->child("direction").setValue(gaze);
    camera->child("up").setValue(up);
  }

  if (camera->hasChild("stereoMode"))
      camera->child("stereoMode").setValue((int)optStereoMode);

  if (camera->hasChild("interpupillaryDistance"))
    camera->child("interpupillaryDistance").setValue(optInterpupillaryDistance);
}

void BatchContext::importFiles(sg::NodePtr world)
{
  importedModels = createNode("importXfm", "transform");
  frame->child("world").add(importedModels);
  if (!sgFileCameras)
    cameras = std::make_shared<CameraMap>();

  for (auto file : filesToImport) {
    try {
      rkcommon::FileName fileName(file);
      if (fileName.ext() == "sg") {
        importScene(shared_from_this(), fileName);
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
          // Could be any type of importer.  Need to pass the MaterialRegistry,
          // importer will use what it needs.
          importer->setFb(frame->childAs<sg::FrameBuffer>("framebuffer"));
          importer->setMaterialRegistry(baseMaterialRegistry);
          if (sgFileCameras) {
            importer->importCameras = false;
            importer->setCameraList(sgFileCameras);
            for (auto i = sgFileCameras->begin() + 1; i != sgFileCameras->end();
                 i++)
              cameras->operator[](i->first) = i->second;
          } else if (cameras) {
            importer->importCameras = true;
            importer->setCameraList(cameras);
          }
          importer->setLightsManager(lightsManager);
          importer->setArguments(studioCommon.argc, (char**)studioCommon.argv);
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

  for (;;) {
    size_t numTasksExecuted = 0;

    if (optDoAsyncTasking) {
      numTasksExecuted += scheduler->background()->executeAllTasksAsync();

      if (numTasksExecuted == 0) {
        if (scheduler->background()->wait() > 0) {
          continue;
        }
      }
    } else {
      numTasksExecuted += scheduler->background()->executeAllTasksSync();
    }

    numTasksExecuted += scheduler->ospray()->executeAllTasksSync();
    numTasksExecuted += scheduler->studio()->executeAllTasksSync();

    if (numTasksExecuted == 0) {
      break;
    }
  }

  filesToImport.clear();

  // Initializes time range for newly imported models
  animationManager->init();
}
