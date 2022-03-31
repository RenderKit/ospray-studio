// Copyright 2009-2022 Intel Corporation
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
    for (auto &cs : j)
      cameraStack.push_back(cs.at("cameraToWorld"));
  }

  if (parseCommandLine()) {
    std::cout << "...importing files!" << std::endl;
    refreshRenderer();
    refreshScene(true);

    if (cameras.size())
      std::cout << "List of imported scene cameras:\n";
    for (int c = 1; c <= cameras.size(); ++c) {
      std::cout
          << c << ": "
          << cameras[c - 1]->child("uniqueCameraName").valueAs<std::string>()
          << std::endl;
    }

    for (int cameraIdx = cameraRange.lower; cameraIdx <= cameraRange.upper;
         ++cameraIdx) {
      resetFileId = true;
      refreshCamera(cameraIdx);

      // if a camera stack is present loop over every entry of camera stack for
      // every camera
      if (cameraStack.size())
        for (auto &c : cameraStack) {
          finalCameraView = std::make_shared<affine3f>(c);
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
    "--format",
    optImageFormat,
    "Sets the image format for color components(RGBA)"
  )->check(CLI::IsMember({"png", "jpg", "ppm", "pfm", "exr", "hdr"}));
  app->add_option(
    "--image",
    optImageName,
    "Sets the image name (inclusive of path and filename)"
  );
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
  app->add_flag(
    "--denoiser",
    optDenoiser,
    "Set the denoiser"
  );
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
    "--saveAlbedo",
    saveAlbedo,
    "Save albedo values"
  );
  app->add_flag(
    "--saveDepth",
    saveDepth,
    "Save depth values"
  );
  app->add_flag(
    "--saveNormal",
    saveNormal,
    "Save normal values" 
  );
  app->add_flag(
    "--saveLayers",
    saveLayersSeparatly,
    "Save layers in separate files");
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
    "--camera",
    cameraRange,
    "Set the camera index to use"
  )->check(CLI::PositiveNumber);
  app->add_option(
    "--cameras",
    [&](const std::vector<std::string> val) {
      cameraRange.lower = std::stoi(val[0]);
      cameraRange.upper = std::stoi(val[1]);
      return true;
    },
    "Set the camera range"
  )->expected(2);
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
  app->add_option(
    "--bgColor",
    [&](const std::vector<std::string> val) {
      bgColor = rgba(std::stof(val[0]),
        std::stof(val[1]),
        std::stof(val[2]),
        std::stof(val[3]));
      return true;
    },
    "Set the renderer background color"
    )->expected(4);
}

bool BatchContext::parseCommandLine()
{
  int ac = studioCommon.argc;
  const char **av = studioCommon.argv;

  std::shared_ptr<CLI::App> app = std::make_shared<CLI::App>("OSPRay Studio Batch");
  StudioContext::addToCommandLine(app);
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
  auto &renderer = frame->childAs<sg::Renderer>("renderer");

  if (optPF >= 0)
    renderer.createChild("pixelFilter", "int", optPF);

  renderer.child("pixelSamples").setValue(optSPP);
  renderer.child("varianceThreshold").setValue(optVariance);

  if (renderer.hasChild("maxContribution") && maxContribution < (float)math::inf)
    renderer["maxContribution"].setValue(maxContribution);

  renderer["backgroundColor"] = bgColor;
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
  if (cameraIdx <= cameras.size() && cameraIdx > 0) {
    std::cout << "Loading camera from index: " << std::to_string(cameraIdx)
              << std::endl;
    selectedSceneCamera = cameras[cameraIdx - 1];
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

    int screenshotFlags = saveLayersSeparatly << 3 | saveNormal << 2
        | saveDepth << 1 | saveAlbedo;

    frame->saveFrame(filename, screenshotFlags);

    this->outputFilename = filename;
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
  auto &camera = frame->child("camera");
  // use given camera view if present
  if (finalCameraView) {
    affine3f cameraToWorld = *finalCameraView;
    camera["position"] = xfmPoint(cameraToWorld, vec3f(0, 0, 0));
    camera["direction"] = xfmVector(cameraToWorld, vec3f(0, 0, -1));
    camera["up"] = xfmVector(cameraToWorld, vec3f(0, 1, 0));
  }
  // if no camera  view or scene camera is selected calculate a default view
  else if (cameraRange.lower == 0) {
    auto worldBounds = getSceneBounds();
    auto worldDiag = length(worldBounds.size());
    auto centerTranslation = AffineSpace3f::translate(-worldBounds.center());
    auto translation = AffineSpace3f::translate(vec3f(0, 0, -worldDiag));
    auto cameraToWorld = rcp(translation * centerTranslation);

    camera["position"] = xfmPoint(cameraToWorld, vec3f(0, 0, 0));
    camera["direction"] = xfmVector(cameraToWorld, vec3f(0, 0, -1));
    camera["up"] = xfmVector(cameraToWorld, vec3f(0, 1, 0));
  }

  if (cmdlCam) {
    camera["position"] = pos;
    camera["direction"] = gaze;
    camera["up"] = up;
  }

  if (camera.hasChild("stereoMode"))
    camera["stereoMode"] = optStereoMode;

  if (camera.hasChild("interpupillaryDistance"))
    camera["interpupillaryDistance"] = optInterpupillaryDistance;
}

void BatchContext::importFiles(sg::NodePtr world)
{
  importedModels = createNode("importXfm", "transform");
  frame->child("world").add(importedModels);
  animationManager = std::shared_ptr<AnimationManager>(new AnimationManager);

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
          importer->setCameraList(cameras);
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
