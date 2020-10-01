// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Batch.h"
// ospray_sg
#include "sg/Frame.h"
#include "sg/fb/FrameBuffer.h"
#include "sg/importer/Importer.h"
#include "sg/renderer/MaterialRegistry.h"
#include "sg/visitors/Commit.h"
#include "sg/visitors/PrintNodes.h"
// rkcommon
#include "rkcommon/utility/SaveImage.h"
// json
#include "sg/JSONDefs.h"

BatchContext::BatchContext(StudioCommon &_common)
    : StudioContext(_common), optImageSize(_common.defaultSize)
{
  frame = sg::createNodeAs<sg::Frame>("main_frame", "frame");

  baseMaterialRegistry = sg::createNodeAs<sg::MaterialRegistry>(
      "baseMaterialRegistry", "materialRegistry");
}

void BatchContext::start()
{
  std::cerr << "Batch mode\n";

  // load plugins //

  for (auto &p : studioCommon.pluginsToLoad)
    pluginManager.loadPlugin(p);

  if (parseCommandLine()) {
    std::cout << "...importing files!" << std::endl;
    refreshScene(true);
    std::cout << "...rendering!" << std::endl;
    render();
    std::cout << "...finished!" << std::endl;
  }
}

bool BatchContext::parseCommandLine()
{
  int argc = studioCommon.argc;
  const char **argv = studioCommon.argv;

  bool retVal = true;
  // Very basic command-line parsing
  // Anything beginning with - is an option.
  // Everything else is considered an import filename
  for (int i = 1; i < argc; i++) {
    const std::string arg = argv[i];
    std::cout << "Arg: " << arg << "\n";
    if (arg.front() == '-') {
      if (arg == "--help") {
        printHelp();
        retVal = false;
      } else if (arg == "-r" || arg == "--renderer") {
        optRendererTypeStr = argv[i + 1];
        removeArgs(argc, argv, i, 2);
        --i;
      } else if (arg == "-c" || arg == "--camera") {
        optCameraTypeStr = argv[i + 1];
        removeArgs(argc, argv, i, 2);
        --i;
      } else if (arg == "-vp") {
        vec3f posVec;
        posVec.x = atof(argv[i + 1]);
        posVec.y = atof(argv[i + 2]);
        posVec.z = atof(argv[i + 3]);
        removeArgs(argc, argv, i, 4);
        --i;
        pos     = posVec;
        cmdlCam = true;
      } else if (arg == "-vu") {
        vec3f upVec;
        upVec.x = atof(argv[i + 1]);
        upVec.y = atof(argv[i + 2]);
        upVec.z = atof(argv[i + 3]);
        removeArgs(argc, argv, i, 4);
        --i;
        up      = upVec;
        cmdlCam = true;
      } else if (arg == "-vi") {
        vec3f gazeVec;
        gazeVec.x = atof(argv[i + 1]);
        gazeVec.y = atof(argv[i + 2]);
        gazeVec.z = atof(argv[i + 3]);
        removeArgs(argc, argv, i, 4);
        --i;
        gaze    = gazeVec;
        cmdlCam = true;
      } else if (arg == "-id" || arg == "--interpupillaryDistance") {
        optInterpupillaryDistance = max(0.0, atof(argv[i + 1]));
        removeArgs(argc, argv, i, 2);
        --i;
      } else if (arg == "-sm" || arg == "--stereoMode") {
        optStereoMode = max(0, atoi(argv[i + 1]));
        removeArgs(argc, argv, i, 2);
        --i;
      } else if (arg == "-i" || arg == "--image") {
        optImageName = argv[i + 1];
        removeArgs(argc, argv, i, 2);
        --i;
      } else if (arg == "-s" || arg == "--size") {
        auto x       = max(0, atoi(argv[i + 1]));
        auto y       = max(0, atoi(argv[i + 2]));
        optImageSize = vec2i(x, y);
        removeArgs(argc, argv, i, 3);
        --i;
      } else if (arg == "-spp" || arg == "--samples") {
        optSPP = max(1, atoi(argv[i + 1]));
        removeArgs(argc, argv, i, 2);
        --i;
      } else if (arg == "-pf" || arg == "--pixelfilter") {
        optPF = max(0, atoi(argv[i + 1]));
        removeArgs(argc, argv, i, 2);
        --i;
      } else if (arg == "-oidn" || arg == "--denoiser") {
        if (studioCommon.denoiserAvailable)
          optDenoiser = min(2, max(0, atoi(argv[i + 1])));
        else
          std::cout << " Denoiser not enabled. Check OSPRay module.\n";

        removeArgs(argc, argv, i, 2);
        --i;
      } else if (arg == "-g" || arg == "--grid") {
        auto x        = max(0, atoi(argv[i + 1]));
        auto y        = max(0, atoi(argv[i + 2]));
        auto z        = max(0, atoi(argv[i + 3]));
        optGridSize   = vec3i(x, y, z);
        optGridEnable = true;
        removeArgs(argc, argv, i, 4);
        --i;
      } else {
        // Unknown option, can't continue
        std::cout << " Unknown option: " << arg << std::endl;
        retVal = false;
        break;
      }
    } else
      filesToImport.push_back(arg);
  }

  return retVal;
}

void BatchContext::render()
{
  frame->createChild("renderer", "renderer_" + optRendererTypeStr);
  frame->createChild("camera", "camera_" + optCameraTypeStr);
  baseMaterialRegistry->updateMaterialList(optRendererTypeStr);
  frame->child("renderer")
      .createChildData("material", baseMaterialRegistry->cppMaterialList);
  // Set the frame "windowSize", it will create the right sized framebuffer
  frame->child("windowSize") = optImageSize;

  if (optPF >= 0)
    frame->child("renderer").createChild("pixelFilter", "int", optPF);

  auto &frameBuffer = frame->childAs<sg::FrameBuffer>("framebuffer");

  // If using the denoiser, set the framebuffer to allow it.
  if (studioCommon.denoiserAvailable && optDenoiser)
    frameBuffer["allowDenoising"] = true;

  // Create a default ambient light
  frame->child("world").createChild("materialref", "reference_to_material", 0);

  if (optGridEnable) {
    // Determine world bounds to calculate grid offsets
    frame->child("world").render();
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
          auto copy = sg::createNode(nodeName,
              "Transform",
              affine3f::translate(vec3f(tx * x, ty * y, tz * z)));
          copy->add(importedModels);
          frame->child("world").add(copy);
        }
  }

  frame->child("world").render();

  // Update camera based on world bounds after import
  if (!sgScene)
    arcballCamera.reset(
        new ArcballCamera(frame->child("world").bounds(), optImageSize));

  std::ifstream cams("cams.json");
  if (cams) {
    std::vector<CameraState> cameraStack;
    nlohmann::json j;
    cams >> j;
    cameraStack = j.get<std::vector<CameraState>>();
    CameraState cs = cameraStack.front();
    arcballCamera->setState(cs);
  }

  auto &camera = frame->child("camera");
  if (camera.hasChild("aspect"))
    camera["aspect"] = optImageSize.x / (float)optImageSize.y;

  if (cmdlCam) {
    camera["position"] = pos;
    camera["direction"] = normalize(gaze - pos);
    camera["up"] = up;
  }

  camera["stereoMode"] = optStereoMode;
  camera["interpupillaryDistance"] = optInterpupillaryDistance;

  updateCamera();

  // frame->child("world").createChild("light", "ambient");
  frame->child("navMode") = false;

  frame->render();

  // this forces a commit, needed for scene imports (.sg)
  // TODO: there is a desync between imported nodes marked as committed vs
  // actually committed to OSPRay. This is just a bandaid
  frame->traverse<CommitVisitor>(true);

  // Accumulate several frames
  for (auto i = 0; i < optSPP - 1; i++) {
    frame->immediatelyWait = true;
    frame->startNewFrame();
  }

  // Only denoise the final frame
  // XXX TODO if optDenoiser == 2, save both the noisy and denoised color
  // buffers.  How best to do that since the frame op will alter the final
  // buffer?
  if (studioCommon.denoiserAvailable && optDenoiser)
    frame->denoiseFB = true;
  frame->immediatelyWait = true;
  frame->startNewFrame();

  auto size = frameBuffer["size"].valueAs<vec2i>();
  const void *pixels = frame->mapFrame();

  if (frameBuffer.hasFloatFormat()) {
    optImageName += ".pfm";
    rkcommon::utility::writePFM(optImageName, size.x, size.y, (vec4f *)pixels);
  } else {
    optImageName += ".ppm";
    rkcommon::utility::writePPM(
        optImageName, size.x, size.y, (uint32_t *)pixels);
  }

  frame->unmapFrame((void *)pixels);

  std::cout << "\nresult saved to '" << optImageName << "'\n";
}

void BatchContext::refreshScene(bool resetCam)
{
  // Check that the frame contains a world, if not create one
  auto world = frame->hasChild("world") ? frame->childNodeAs<sg::Node>("world")
                                        : sg::createNode("world", "world");

  world->createChild(
      "materialref", "reference_to_material", defaultMaterialIdx);

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
  importedModels = createNode("importXfm", "transform", affine3f{one});
  frame->child("world").add(importedModels);

  for (auto file : filesToImport) {
    try {
      rkcommon::FileName fileName(file);
      if (fileName.ext() == "sg") {
        importScene(shared_from_this(), fileName);
        sgScene = true;
      } else {
        std::string nodeName = fileName.base() + "_importer";
        std::cout << "Importing: " << file << std::endl;

        auto importer = sg::getImporter(file);
        if (importer != "") {
          auto &imp =
              importedModels->createChildAs<sg::Importer>(nodeName, importer);
          // Could be any type of importer.  Need to pass the MaterialRegistry,
          // importer will use what it needs.
          imp.setFileName(fileName);
          imp.setMaterialRegistry(baseMaterialRegistry);
          imp.importScene();
        }
      }
    } catch (...) {
      std::cerr << "Failed to open file '" << file << "'!\n";
    }
  }

  filesToImport.clear();
}

void BatchContext::printHelp()
{
  std::cout <<
      R"text(
./ospStudio batch [parameters] [scene_files]

ospStudio batch specific parameters:
   -i     --image [baseFilename] (default 'ospBatch')
            base name of saved image
   -s     --size [x y] (default 1024x768)
            image size
   -spp   --samples [int] (default 32)
            samples per pixel
   -pf    --pixelfilter (default gauss)
            (0=point, 1=box, 2=gauss, 3=mitchell, 4=blackman_harris)
   -r     --renderer [type] (default "scivis")
            rendererType scivis or pathtracer
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
