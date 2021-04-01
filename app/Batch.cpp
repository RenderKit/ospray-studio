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

static bool resetFileId = false;

BatchContext::BatchContext(StudioCommon &_common)
    : StudioContext(_common), optImageSize(_common.defaultSize)
{
  frame->child("scaleNav").setValue(1.f);
}

void BatchContext::start()
{
  std::cerr << "Batch mode\n";

  // load plugins //

  for (auto &p : studioCommon.pluginsToLoad)
    pluginManager.loadPlugin(p);

  if (parseCommandLine()) {
    std::cout << "...importing files!" << std::endl;
    refreshRenderer();
    refreshScene(true);
    if (!useCameraRange) {
      resetFileId = true;
      refreshCamera(cameraDef);
      render();
      if (animate) {
        std::cout << "..rendering animation!" << std::endl;
        renderAnimation();
      } else
        renderFrame();
    } else {
      for (int cameraIdx = cameraRange.lower; cameraIdx <= cameraRange.upper;
           ++cameraIdx) {
        resetFileId = true;
        refreshCamera(cameraIdx);
        render();
        if (animate) {
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

bool BatchContext::parseCommandLine()
{
  int argc = studioCommon.argc;
  const char **argv = studioCommon.argv;
  int argIndex = 1;

  volumeParams = std::make_shared<sg::VolumeParams>();

  auto argAvailability = [&](std::string switchArg, int nComp) {
    if (argc >= argIndex + nComp)
      return true;
    std::cout << "Missing argument value for : " << switchArg << std::endl;
    return false;
  };

  while (argIndex < argc) {
    std::string switchArg(argv[argIndex++]);

    if (switchArg == "--help") {
      printHelp();
      return 0;
    } else if (switchArg == "-r" || switchArg == "--renderer") {
      if (argAvailability(switchArg, 1))
        optRendererTypeStr = argv[argIndex++];

    } else if (switchArg == "-c" || switchArg == "--camera") {
      if (argAvailability(switchArg, 1))
        optCameraTypeStr = argv[argIndex++];

    } else if (switchArg == "-vp") {
      if (argAvailability(switchArg, 3)) {
        vec3f posVec;
        posVec.x = atof(argv[argIndex++]);
        posVec.y = atof(argv[argIndex++]);
        posVec.z = atof(argv[argIndex++]);
        pos = posVec;
        cmdlCam = true;
      }

    } else if (switchArg == "-vu") {
      if (argAvailability(switchArg, 3)) {
        vec3f upVec;
        upVec.x = atof(argv[argIndex++]);
        upVec.y = atof(argv[argIndex++]);
        upVec.z = atof(argv[argIndex++]);
        up = upVec;
        cmdlCam = true;
      }

    } else if (switchArg == "-f" || switchArg == "--format") {
      if (argAvailability(switchArg, 1))
        optImageFormat = argv[argIndex++];

    } else if (switchArg == "-i" || switchArg == "--image") {
      if (argAvailability(switchArg, 1))
        optImageName = argv[argIndex++];

    } else if (switchArg == "-vi") {
      if (argAvailability(switchArg, 3)) {
        vec3f gazeVec;
        gazeVec.x = atof(argv[argIndex++]);
        gazeVec.y = atof(argv[argIndex++]);
        gazeVec.z = atof(argv[argIndex++]);
        gaze = gazeVec;
        cmdlCam = true;
      }

    } else if (switchArg == "-id" || switchArg == "--interpupillaryDistance") {
      if (argAvailability(switchArg, 1))
        optInterpupillaryDistance = max(0.0, atof(argv[argIndex++]));

    } else if (switchArg == "-sm" || switchArg == "--stereoMode") {
      if (argAvailability(switchArg, 1))
        optStereoMode = max(0, atoi(argv[argIndex++]));

    } else if (switchArg == "-s" || switchArg == "--size") {
      if (argAvailability(switchArg, 2)) {
        auto x = max(0, atoi(argv[argIndex++]));
        auto y = max(0, atoi(argv[argIndex++]));
        optImageSize = vec2i(x, y);
      }

    } else if (switchArg == "-spp" || switchArg == "--samples") {
      if (argAvailability(switchArg, 1))
        optSPP = max(1, atoi(argv[argIndex++]));

    } else if (switchArg == "-vt" || switchArg == "--variance") {
      if (argAvailability(switchArg, 1))
        optVariance = max(0.0, atof(argv[argIndex++]));

    } else if (switchArg == "-pf" || switchArg == "--pixelfilter") {
      if (argAvailability(switchArg, 1))
        optPF = max(0, atoi(argv[argIndex++]));

    } else if (switchArg == "-oidn" || switchArg == "--denoiser") {
      if (studioCommon.denoiserAvailable) {
        if (argAvailability(switchArg, 1))
          optDenoiser = min(2, max(0, atoi(argv[argIndex++])));
      } else {
        std::cout << " Denoiser not enabled. Check OSPRay module.\n";
        argIndex++;
      }
    } else if (switchArg == "-g" || switchArg == "--grid") {
      if (argAvailability(switchArg, 3)) {
        auto x = max(0, atoi(argv[argIndex++]));
        auto y = max(0, atoi(argv[argIndex++]));
        auto z = max(0, atoi(argv[argIndex++]));
        optGridSize = vec3i(x, y, z);
        optGridEnable = true;
      }
    } else if (switchArg == "-a" || switchArg == "--albedo") {
      saveAlbedo = true;
    } else if (switchArg == "-d" || switchArg == "--depth") {
      saveDepth = true;
    } else if (switchArg == "-n" || switchArg == "--normal") {
      saveNormal = true;
    } else if (switchArg == "-l" || switchArg == "--layers") {
      saveLayers = true;
    } else if (switchArg == "-m" || switchArg == "--metadata") {
      saveMetaData = true;
    } else if (switchArg == "-fps" || switchArg == "--speed") {
      if (argAvailability(switchArg, 1))
        fps = atoi(argv[argIndex++]);
      animate = true;
    } else if (switchArg == "-fr" || switchArg == "--force") {
      forceRewrite = true;
    } else if (switchArg == "-cam" || switchArg == "--camera") {
      if (argAvailability(switchArg, 1)) {
        cameraDef = std::stoi(argv[argIndex++]);
        if (cameraDef < 0) {
          std::cout << "unsupported camera index specified " << std::endl;
          return false;
        }
      }
      if (!cameraDef)
        std::cout
            << "using default ospray camera, to use imported definition camera indices begins from 1"
            << std::endl;
    } else if (switchArg == "-cams" || switchArg == "--cameras") {
      if (argAvailability(switchArg, 2)) {
        auto x = atoi(argv[argIndex++]);
        auto y = atoi(argv[argIndex++]);
        cameraRange.lower = x;
        cameraRange.upper = y;
        useCameraRange = true;
      }
      if (!cameraRange.lower && !cameraRange.upper)
        std::cout
            << "using default ospray camera, to use imported definition camera indices begins from 1"
            << std::endl;
    } else if (switchArg == "-rn" || switchArg == "--range") {
      if (argAvailability(switchArg, 2)) {
        auto x = atoi(argv[argIndex++]);
        auto y = atoi(argv[argIndex++]);
        framesRange.lower = x;
        framesRange.upper = y;
      }
    }
    // volume parameters 
    else if (switchArg == "--dimensions" || switchArg == "-dim") {
      if (argAvailability(switchArg, 3)) {
      const std::string dimX(argv[argIndex++]);
      const std::string dimY(argv[argIndex++]);
      const std::string dimZ(argv[argIndex++]);
      auto dimensions = vec3i(std::stoi(dimX), std::stoi(dimY), std::stoi(dimZ));
      volumeParams->createChild("dimensions", "vec3i", dimensions);
      }
    } else if (switchArg == "--gridSpacing" || switchArg == "-gs") {
      if (argAvailability(switchArg, 3)) {
      const std::string gridSpacingX(argv[argIndex++]);
      const std::string gridSpacingY(argv[argIndex++]);
      const std::string gridSpacingZ(argv[argIndex++]);
      auto gridSpacing =
          vec3f(stof(gridSpacingX), stof(gridSpacingY), stof(gridSpacingZ));
      volumeParams->createChild("gridSpacing", "vec3f", gridSpacing);
      }
    } else if (switchArg == "--gridOrigin" || switchArg == "-go") {
      if (argAvailability(switchArg, 3)) {
      const std::string gridOriginX(argv[argIndex++]);
      const std::string gridOriginY(argv[argIndex++]);
      const std::string gridOriginZ(argv[argIndex++]);
      auto gridOrigin =
          vec3f(stof(gridOriginX), stof(gridOriginY), stof(gridOriginZ));
      volumeParams->createChild("gridOrigin", "vec3f", gridOrigin);
      }
    } else if (switchArg == "--voxelType" || switchArg == "-vt") {
      if (argAvailability(switchArg, 1)) {
      auto voxelTypeStr = std::string(argv[argIndex++]);
      auto it           = sg::volumeVoxelType.find(voxelTypeStr);
      if (it != sg::volumeVoxelType.end()) {
        auto voxelType = it->second;
        volumeParams->createChild("voxelType", "int", (int)voxelType);
      } else {
        throw std::runtime_error("improper -voxelType format requested");
      }
      }
    } else if (switchArg.front() == '-') {
      std::cout << " Unknown option: " << switchArg << std::endl;
      break;
    }
    else {
      filesToImport.push_back(switchArg);
    }
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
}

void BatchContext::refreshCamera(int cameraIdx)
{
  if (cameraIdx <= cameras.size() && cameraIdx > 0) {
    std::cout << "Loading camera from index: " << std::to_string(cameraIdx)
              << std::endl;
    selectedSceneCamera = cameras[cameraIdx - 1];
    animateCamera = selectedSceneCamera->nodeAs<sg::Camera>()->animate;

    auto &camera = frame->createChildAs<sg::Camera>(
        "camera", selectedSceneCamera->subType());
    for (auto &c : selectedSceneCamera->children())
      camera.add(c.second);

  } else {
    std::cout << "No cameras imported or invalid camera index specified"
              << std::endl;
    selectedSceneCamera = createNode(
        "camera" + std::to_string(cameraIdx), "camera_" + optCameraTypeStr);
    frame->add(selectedSceneCamera);
  }
  // create unique cameraId for every camera
  auto &cameraXfm = selectedSceneCamera->parents().front();
  cameraXfm->createChild(
      "cameraId", "string", "Camera_" + std::to_string(cameraIdx));
}

void BatchContext::render()
{
  // Set the frame "windowSize", it will create the right sized framebuffer
  frame->child("windowSize") = optImageSize;

  auto &frameBuffer = frame->childAs<sg::FrameBuffer>("framebuffer");

  // If using the denoiser, set the framebuffer to allow it.
  if (studioCommon.denoiserAvailable && optDenoiser) {
    frameBuffer["floatFormat"] = true;
    frameBuffer.commit();
  }

  frame->child("world").createChild("materialref", "reference_to_material", 0);
  if(saveMetaData)
    frame->child("world").child("saveMetaData").setValue(true);

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

  // Update camera based on world bounds after import
  if (!sgScene)
    arcballCamera.reset(
        new ArcballCamera(frame->child("world").bounds(), optImageSize));

  std::ifstream cams("cams.json");
  if (cams) {
    std::vector<CameraState> cameraStack;
    JSON j;
    cams >> j;
    cameraStack = j.get<std::vector<CameraState>>();
    CameraState cs = cameraStack.front();
    arcballCamera->setState(cs);
  }

  updateCamera();

  auto &camera = frame->child("camera");
  if (camera.hasChild("aspect"))
    camera["aspect"] = optImageSize.x / (float)optImageSize.y;

  if (cmdlCam) {
    camera["position"] = pos;
    camera["direction"] = normalize(gaze - pos);
    camera["up"] = up;
  }

  if(camera.hasChild("stereoMode"))
  camera["stereoMode"] = optStereoMode;

  if(camera.hasChild("interpupillaryDistance"))
  camera["interpupillaryDistance"] = optInterpupillaryDistance;

  frame->child("navMode") = false;
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

  if (animateCamera) {
    auto newCS = selectedSceneCamera->nodeAs<sg::Camera>()->getState();
    arcballCamera->setState(*newCS);
    updateCamera();
  }
  std::string cameraId{""};
  auto &cameraXfm = selectedSceneCamera->parents().front();
  if (cameraXfm->hasChild("geomId"))
    cameraId = cameraXfm->child("geomId").valueAs<std::string>();
  else
    cameraId = cameraXfm->child("cameraId").valueAs<std::string>();

  static int filenum;
  if (resetFileId) {
    filenum = framesRange.lower;
    resetFileId = false;
  }

  char filenumber[8];
  std::string filename;
  if (!forceRewrite)
    do {
      std::snprintf(filenumber, 8, ".%05d.", filenum++);
      filename = cameraId + "_" + optImageName + filenumber + optImageFormat;
    } while (std::ifstream(filename.c_str()).good());
  else {
    std::snprintf(filenumber, 8, ".%05d.", filenum++);
    filename = cameraId + "_" + optImageName + filenumber + optImageFormat;
  }

  int screenshotFlags = saveMetaData << 4 | saveLayers << 3
      | saveNormal << 2 | saveDepth << 1 | saveAlbedo;

  frame->saveFrame(filename, screenshotFlags);
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

  world->createChild(
      "materialref", "reference_to_material", defaultMaterialIdx);
  if (saveMetaData)
    world->child("saveMetaData").setValue(true);

  if (!filesToImport.empty())
    importFiles(world);

  world->render();

  frame->add(world);

  if (resetCam && !sgScene)
    arcballCamera.reset(
        new ArcballCamera(frame->child("world").bounds(), optImageSize));
  updateCamera();
  auto &fb = frame->childAs<sg::FrameBuffer>("framebuffer");
  fb.resetAccumulation();
}

void BatchContext::updateCamera()
{
  auto &camera = frame->child("camera");

  camera["position"]  = arcballCamera->eyePos();
  camera["direction"] = arcballCamera->lookDir();
  camera["up"]        = arcballCamera->upDir();
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
          if (volumeParams->children().size() > 0) {
            auto vp = importer->getVolumeParams();
            for (auto &c : volumeParams->children()) {
              vp->remove(c.first);
              vp->add(c.second);
            }
          }
          if (animationManager)
            importer->setAnimationList(animationManager->getAnimations());
          importer->importScene();
          world->add(importer);
        }
      }
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
            instace a grid of models)text"
            << std::endl;
  if (studioCommon.denoiserAvailable) {
    std::cout <<
        R"text(
   -oidn  --denoiser [0,1,2] (default 0)
            image denoiser (0 = off, 1 = on, 2 = save both)
)text" << std::endl;
  }
}
