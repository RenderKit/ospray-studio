// Copyright 2009-2021 Intel Corporation
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

  if (parseCommandLine()) {
    std::cout << "...importing files!" << std::endl;
    refreshRenderer();
    refreshScene(true);
    if (!useCameraRange) {
      resetFileId = true;
      bool useCamera = refreshCamera(cameraDef);
      if (useCamera) {
        render();
        if (animate) {
          std::cout << "..rendering animation!" << std::endl;
          renderAnimation();
        } else if (!cameraStack.empty()) {
          std::cout << "..using cams.json camera stack" << std::endl;
          for (auto &cs : cameraStack) {
            arcballCamera->setState(cs);
            updateCamera();
            renderFrame();
          }
        } else
          renderFrame();
      }
    } else {
      for (int cameraIdx = cameraRange.lower; cameraIdx <= cameraRange.upper;
           ++cameraIdx) {
        resetFileId = true;
        bool useCamera = refreshCamera(cameraIdx, true);
        if (useCamera) {
          render();
          if (animate) {
            std::cout << "..rendering animation!" << std::endl;
            renderAnimation();
          } else
            renderFrame();
        }
      }
    }

    std::cout << "...finished!" << std::endl;
    sg::clearAssets();
  }
}

bool BatchContext::parseCommandLine()
{
  int ac = studioCommon.argc;
  const char **av = studioCommon.argv;
  for (int i=1; i<ac; ++i) {
    std::string s = av[i];
    auto it = batchCommandLineAliases.find(s);
    if (it != batchCommandLineAliases.end()) {
      av[i] = it->second;
    }
  }

  CLI::App app{"OSPRay Studio Batch"};
  volumeParams = std::make_shared<sg::VolumeParams>();

  app.add_option(
    "files",
    filesToImport,
    "The list of files to import"
  );
  app.add_option(
    "--renderer",
    optRendererTypeStr,
    "Set the renderer"
  )->check(CLI::IsMember({"scivis", "pathtracer", "ao", "debug"}));
  app.add_option(
    "--camera-type",
    optCameraTypeStr,
    "Set the camera type"
  )->check(CLI::IsMember({"perspective", "orthographic", "panoramic"}));
  app.add_option(
    "--position",
    [&](const std::vector<std::string> val) {
      pos = vec3f(std::stof(val[0]), std::stof(val[1]), std::stof(val[2]));
      cmdlCam = true;
      return true;
    },
    "Set the camera position"
  )->expected(3);
  app.add_option(
    "--view",
    [&](const std::vector<std::string> val) {
      gaze = vec3f(std::stof(val[0]), std::stof(val[1]), std::stof(val[2]));
      cmdlCam = true;
      return true;
    },
    "Set the camera view vector"
  )->expected(3);
  app.add_option(
    "--up",
    [&](const std::vector<std::string> val) {
      up = vec3f(std::stof(val[0]), std::stof(val[1]), std::stof(val[2]));
      cmdlCam = true;
      return true;
    },
    "Set the camera up vector"
  );
  app.add_option(
    "--format",
    optImageFormat,
    "Set the image format"
  )->check(CLI::IsMember({"png", "jpg", "ppm", "pfm", "exr", "hdr"}));
  app.add_option(
    "--image",
    optImageName,
    "Set the image name"
  );
  app.add_option(
    "--interpupillaryDistance",
    optInterpupillaryDistance,
    "Set the interpupillary distance"
  )->check(CLI::PositiveNumber);
  app.add_option(
    "--stereoMode",
    optStereoMode,
    "Set the stereo mode"
  )->check(CLI::PositiveNumber);
  app.add_option(
    "--size",
    [&](const std::vector<std::string> val) {
      optImageSize = vec2i(std::stoi(val[0]), std::stoi(val[1]));
      return true;
    },
    "Set the image size"
  )->expected(2)->check(CLI::PositiveNumber);
  app.add_option(
    "--samples",
    optSPP,
    "Set the samples-per-pixel"
  )->check(CLI::Range(1, 1024));
  app.add_option(
    "--variance",
    optVariance,
    "Set the maximum variance"
  )->check(CLI::NonNegativeNumber);
  app.add_option(
    "--pixelfilter",
    optPF,
    "Set the pixelfilter"
  )->check(CLI::NonNegativeNumber);
  app.add_option_function<int>(
    "--denoiser",
    [&](const int denoiser) {
      if (studioCommon.denoiserAvailable) {
        optDenoiser = denoiser;
        return true;
      }
      return false;
    },
    "Set the denoiser"
  )->check(CLI::Range(0, 2+1));
  app.add_option(
    "--grid",
    [&](const std::vector<std::string> val) {
      optGridSize = vec3i(std::stoi(val[0]), std::stoi(val[1]), std::stoi(val[2]));
      optGridEnable = true;
      return true;
    },
    "Set the camera position"
  )->expected(3);
  app.add_flag(
    "--albedo",
    saveAlbedo,
    "Save albedo values"
  );
  app.add_flag(
    "--depth",
    saveDepth,
    "Save depth values"
  );
  app.add_flag(
    "--normal",
    saveNormal,
    "Save normal values" 
  );
  app.add_flag(
    "--layers",
    saveLayers,
    "Save all layers"
  );
  app.add_flag(
    "--metadata",
    saveMetaData,
    "Save metadata"
  );
  app.add_option(
    "--speed",
    fps,
    "Set the number of frames per second (integer)"
  );
  app.add_flag(
    "--force",
    forceRewrite,
    "Force overwriting saved files if they exist"
  );
  app.add_option(
    "--camera",
    cameraDef,
    "Set the camera index to use"
  )->check(CLI::PositiveNumber);
  app.add_option(
    "--cameras",
    [&](const std::vector<std::string> val) {
      cameraRange.lower = std::stoi(val[0]);
      cameraRange.upper = std::stoi(val[1]);
      useCameraRange = true;
      return true;
    },
    "Set the camera range"
  )->expected(2);
  app.add_option(
    "--range",
    [&](const std::vector<std::string> val) {
      framesRange.lower = std::stoi(val[0]);
      framesRange.upper = std::stoi(val[1]);
      return true;
    },
    "Set the frames range"
  )->expected(2);
  app.add_option(
    "--dimensions",
    [&](const std::vector<std::string> val) {
      auto dimensions = vec3i(std::stoi(val[0]), std::stoi(val[1]), std::stoi(val[2]));
      volumeParams->createChild("dimensions", "vec3i", dimensions);
      return true;
    },
    "Set the dimensions for imported volumes"
  )->expected(3);
  app.add_option(
    "--gridSpacing",
    [&](const std::vector<std::string> val) {
      auto gridSpacing = vec3f(std::stof(val[0]), std::stof(val[1]), std::stof(val[2]));
      volumeParams->createChild("gridSpacing", "vec3f", gridSpacing);
      return true;
    },
    "Set the grid spacing for imported volumes"
  )->expected(3);
  app.add_option(
    "--gridOrigin",
    [&](const std::vector<std::string> val) {
      auto gridOrigin = vec3f(std::stof(val[0]), std::stof(val[1]), std::stof(val[2]));
      volumeParams->createChild("gridSpacing", "vec3f", gridOrigin);
      return true;
    },
    "Set the grid origin for imported volumes"
  )->expected(3);
  app.add_option_function<OSPDataType>(
    "--voxelType",
    [&](const OSPDataType &voxelType) {
      volumeParams->createChild("voxelType", "int", (int)voxelType);
    },
    "Set the voxel type for imported volumes"
  )->transform(CLI::CheckedTransformer(sg::volumeVoxelType));
  app.add_option(
    "--sceneConfig",
    sceneConfig,
    "Set the scene configuration (valid values: dynamic, compact, robust)"
  )->check(CLI::IsMember({"dynamic", "compact", "robust"}));
  app.add_option(
    "--instanceConfig",
    instanceConfig,
    "Set the instance configuration (valid values: dynamic, compact, robust)"
  )->check(CLI::IsMember({"dynamic", "compact", "robust"}));

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
}

bool BatchContext::refreshCamera(int cameraIdx, bool resetArcball)
{
  if(frame->hasChild("camera"))
  frame->remove("camera");

  if (resetArcball)
    arcballCamera.reset(
        new ArcballCamera(frame->child("world").bounds(), optImageSize));

  if (cameraIdx <= cameras.size() && cameraIdx > 0) {
    std::cout << "Loading camera from index: " << std::to_string(cameraIdx)
              << std::endl;
    selectedSceneCamera = cameras[cameraIdx - 1];

    auto &camera = frame->createChildAs<sg::Camera>(
        "camera", selectedSceneCamera->subType());
      
    for (auto &c : selectedSceneCamera->children())
      camera.add(c.second);

    // create unique cameraId for every camera
    auto &cameraParents = selectedSceneCamera->parents();
    if (cameraParents.size()) {
      if (useCameraRange) {
        auto &cameraXfm = cameraParents.front();
        if (cameraXfm->hasChild("geomId"))
          cameraId = cameraXfm->child("geomId").valueAs<std::string>();
        else
          cameraId = ".Camera_" + std::to_string(cameraIdx);
      }

    } else {
      std::cout << "camera not used in GLTF scene" << std::endl;
      return false;
    }

  } else {
    std::cout << "No cameras imported or invalid camera index specified"
              << std::endl;
    selectedSceneCamera = createNode(
        "camera", "camera_" + optCameraTypeStr);
    frame->add(selectedSceneCamera);
  }

  updateCamera();

  return true;
}

void BatchContext::render()
{
  
  // Set the frame "windowSize", it will create the right sized framebuffer
  frame->child("windowSize") = optImageSize;

  auto &frameBuffer = frame->childAs<sg::FrameBuffer>("framebuffer");
  frameBuffer["floatFormat"] = true;
  frameBuffer.commit();

  frame->child("world").createChild("materialref", "reference_to_material", 0);

  if (optGridEnable) {
    // Determine world bounds to calculate grid offsets
    frame->child("world").remove(importedModels);

    box3f bounds = frame->child("world").bounds();
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

  std::ifstream cams("cams.json");
  if (cams) {
    JSON j;
    cams >> j;
    cameraStack = j.get<std::vector<CameraState>>();
  }
}

void BatchContext::renderFrame()
{
  // Only denoise the final frame
  // XXX TODO if optDenoiser == 2, save both the noisy and denoised color
  // buffers.  How best to do that since the frame op will alter the final
  // buffer?
  if (studioCommon.denoiserAvailable && optDenoiser)
    frame->denoiseFB = true;
  frame->immediatelyWait = true;
  frame->startNewFrame();

  if (selectedSceneCamera->nodeAs<sg::Camera>()->animate || cameraDef > 0) {
    auto newCS = selectedSceneCamera->nodeAs<sg::Camera>()->getState();
    arcballCamera->setState(*newCS);
    updateCamera();
    frame->cancelFrame();
    frame->startNewFrame();
  }

  static int filenum;
  if (resetFileId) {
    filenum = framesRange.lower;
    resetFileId = false;
  }

  std::string filename;
  char filenumber[8];
  if (!forceRewrite)
    do {
      std::snprintf(filenumber, 8, ".%05d.", filenum++);
      filename = optImageName + cameraId + filenumber + optImageFormat;
    } while (std::ifstream(filename.c_str()).good());
  else {
    std::snprintf(filenumber, 8, ".%05d.", filenum++);
    filename = optImageName + cameraId + filenumber + optImageFormat;
  }

  int screenshotFlags = saveLayers << 3
      | saveNormal << 2 | saveDepth << 1 | saveAlbedo;

  frame->saveFrame(filename, screenshotFlags);

  this->outputFilename = filename;
  
  pluginManager->main(shared_from_this());
}

void BatchContext::renderAnimation()
{
  float animationTime = animationManager->getTimeRange().upper;
  float step = 1.f / fps;
  float time = animationManager->getTimeRange().lower;

  if (!framesRange.empty() && framesRange.upper) {
    time += step * framesRange.lower;
    animationTime = step * framesRange.upper;
  }
  animationTime += 1e-6;

  while (time <= animationTime) {
    animationManager->update(time);
    renderFrame();
    time += step;
  }
}

void BatchContext::refreshScene(bool resetCam)
{
  // Check that the frame contains a world, if not create one
  auto world = frame->hasChild("world") ? frame->childNodeAs<sg::Node>("world")
                                        : sg::createNode("world", "world");
  if (sceneConfig == "dynamic")
    world->child("dynamicScene").setValue(true);
  else if (sceneConfig == "compact")
    world->child("compactMode").setValue(true);
  else if (sceneConfig == "robust")
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

  if (resetCam && !sgScene)
    arcballCamera.reset(
        new ArcballCamera(frame->child("world").bounds(), optImageSize));

  auto &fb = frame->childAs<sg::FrameBuffer>("framebuffer");
  fb.resetAccumulation();
}

void BatchContext::updateCamera()
{
  frame->currentAccum = 0;
  auto &camera = frame->child("camera");

  camera["position"] = arcballCamera->eyePos();
  camera["direction"] = arcballCamera->lookDir();
  camera["up"] = arcballCamera->upDir();

  if (camera.hasChild("aspect"))
    camera["aspect"] = optImageSize.x / (float)optImageSize.y;

  if (cmdlCam) {
    camera["position"] = pos;
    camera["direction"] = normalize(gaze - pos);
    camera["up"] = up;
  }

  if (camera.hasChild("stereoMode"))
    camera["stereoMode"] = optStereoMode;

  if (camera.hasChild("interpupillaryDistance"))
    camera["interpupillaryDistance"] = optInterpupillaryDistance;
}

void BatchContext::setCameraState(CameraState &cs)
{
  arcballCamera->setState(cs);
}

void BatchContext::importFiles(sg::NodePtr world)
{
  importedModels = createNode("importXfm", "transform");
  frame->child("world").add(importedModels);
  if (animate)
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
          // Could be any type of importer.  Need to pass the MaterialRegistry,
          // importer will use what it needs.
          importer->setFb(frame->childAs<sg::FrameBuffer>("framebuffer"));
          importer->setMaterialRegistry(baseMaterialRegistry);
          importer->setCameraList(cameras);
          importer->setLightsManager(lightsManager);
          importer->setArguments(studioCommon.argc, (char**)studioCommon.argv);
          if (volumeParams->children().size() > 0) {
            auto vp = importer->getVolumeParams();
            for (auto &c : volumeParams->children()) {
              vp->remove(c.first);
              vp->add(c.second);
            }
          }
          if (animationManager)
            importer->setAnimationList(animationManager->getAnimations());
          if (instanceConfig == "dynamic")
            importer->setInstanceConfiguration(
                sg::InstanceConfiguration::DYNAMIC);
          else if (instanceConfig == "compact")
            importer->setInstanceConfiguration(
                sg::InstanceConfiguration::COMPACT);
          else if (instanceConfig == "robust")
            importer->setInstanceConfiguration(
                sg::InstanceConfiguration::ROBUST);
          importer->importScene();
          world->add(importer);
        }
      }
    } catch (const std::exception &e) {
      std::cerr << "Failed to open file '" << file << "'!\n";
      std::cerr << "   " << e.what() << std::endl;
    } catch (...) {
      std::cerr << "Failed to open file '" << file << "'!\n";
    }
  }

  filesToImport.clear();
  if (animationManager)
    animationManager->init();
}

void BatchContext::printHelp()
{
  std::cout <<
      R"text(
./ospStudio batch [parameters] [scene_files]

ospStudio batch specific parameters:
   -fps  --speed
   -fr   --forceRewrite
         force rewrite on existing saved files
   -rn   --range [start end] for eg : [10 20]
         range of frames to be rendered
         This should be determined by the user based on specified `fps` and total animation time.
   -cam  --camera 
         In case of mulitple imported cameras specify which camera definition to use, counting starts from 1
         0 here would use default camera implementation
   -cams  --cameras 
         In case of mulitple imported cameras specify which camera-range to use, counting starts from 1
         for eg. a valid range would be [1 7]
   -a    --albedo
   -d    --depth
   -n    --normal
   -m    --metadata
   -l    --layers
   -f    --format (default png)
          format for saving the image
          (sg, exr, hdr, jpg, pfm,png, ppm)
   -i     --image [baseFilename] (default 'ospBatch')
            base name of saved image
   -s     --size [x y] (default 1024x768)
            image size
   -spp   --samples [int] (default 32)
            samples per pixel
   -pf    --pixelfilter (default gauss)
            (0=point, 1=box, 2=gauss, 3=mitchell, 4=blackman_harris)
   -r     --renderer [type] (default "pathtracer")
            rendererType scivis, ao, or pathtracer
   -c     --camera [type] (default "perspective")
            cameraType perspective or panoramic
   -vp    [x y z] camera position  
   -vu    [x y z] camera up  
   -vi    [x y z] camera look-at  
   -sm    --stereoMode 0=none, 1=left, 2=right, 3=side-by-side, 4=top-bottom
   -id    --interpupillaryDistance
   -g     --grid [x y z] (default 1 1 1, single instance)
            instace a grid of models
   -sc    --sceneConfig(default is the static BVH build of embree)
          set global scene configuration params
          valid values are dynamic, compact and robust
   -ic    --instanceConfig(default is the static BVH build of embree)
          set instance scene configuration params
          valid values are dynamic, compact and robust)text"
            << std::endl;
  if (studioCommon.denoiserAvailable) {
    std::cout <<
        R"text(
   -oidn  --denoiser [0,1,2] (default 0)
            image denoiser (0 = off, 1 = on, 2 = save both)
)text" << std::endl;
  }
}
