// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Batch.h"
// ospray_sg
#include "sg/Frame.h"
#include "sg/importer/Importer.h"
#include "sg/renderer/MaterialRegistry.h"
#include "sg/visitors/PrintNodes.h"
// rkcommon
#include "rkcommon/utility/SaveImage.h"
// cerealization
#include <cereal/archives/json.hpp>
#include <cereal/types/memory.hpp>
#include <cereal/types/vector.hpp>

// Batch mode entry point
void start_Batch_mode(int argc, const char *argv[])
{
  std::cerr << "Batch mode\n";

  bool denoiser = ospLoadModule("denoiser") == OSP_NO_ERROR;

  auto batch = make_unique<BatchContext>(vec2i(1024, 768), denoiser);
  if (batch->parseCommandLine(argc, argv)) {
    std::cout << "...importing files!" << std::endl;
    batch->importFiles();
    std::cout << "...rendering!" << std::endl;
    batch->render();
    std::cout << "...finished!" << std::endl;
    batch.reset();
  }
}

BatchContext::BatchContext(const vec2i &imageSize, bool denoiser)
    : optImageSize(imageSize), denoiserAvailable(denoiser)
{
  frame_ptr = sg::createNodeAs<sg::Frame>("main_frame", "frame");

  baseMaterialRegistry = sg::createNodeAs<sg::MaterialRegistry>(
      "baseMaterialRegistry", "materialRegistry");
  baseMaterialRegistry->addNewSGMaterial("obj");
}

bool BatchContext::parseCommandLine(int &argc, const char **&argv)
{
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
      } else if (arg == "-oidn" || arg == "--denoiser") {
        if (denoiserAvailable)
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
  auto &frame = *frame_ptr;

  frame.createChild("renderer", "renderer_" + optRendererTypeStr);
  frame.createChild("camera", "camera_" + optCameraTypeStr);
  baseMaterialRegistry->updateMaterialList(optRendererTypeStr);
  frame["renderer"].createChildData("material",
                                    baseMaterialRegistry->cppMaterialList);

  frame["framebuffer"]["size"] = optImageSize;

  // Create a default ambient light
  frame["world"].createChild("materialref", "reference_to_material", 0);

  if (!optGridEnable) {
    // Add imported models
    frame["world"].add(importedModels);
  } else {
    // Determine world bounds to calculate grid offsets
    frame["world"].add(importedModels);
    frame["world"].render();
    frame["world"].remove(importedModels);

    box3f bounds = frame["world"].bounds();
    float tx     = bounds.size().x * 1.2f;
    float ty     = bounds.size().y * 1.2f;
    float tz     = bounds.size().z * 1.2f;

    for (auto z = 0; z < optGridSize.z; z++)
      for (auto y = 0; y < optGridSize.y; y++)
        for (auto x = 0; x < optGridSize.x; x++) {
          auto nodeName = "copy_" + std::to_string(x) + ":" +
                          std::to_string(y) + ":" + std::to_string(z) + "_xfm";
          auto copy = sg::createNode(
              nodeName,
              "Transform",
              affine3f::translate(vec3f(tx * x, ty * y, tz * z)));
          copy->add(importedModels);
          frame["world"].add(copy);
        }
  }

  frame["world"].render();

  // Update camera based on world bounds after import
  arcballCamera.reset(new ArcballCamera(frame["world"].bounds(), optImageSize));

  std::ifstream cams("cams.json");
  if (cams) {
    std::vector<CameraState> cameraStack;
    cereal::JSONInputArchive iarchive(cams);
    iarchive(cameraStack);
    CameraState cs = cameraStack.front();
    arcballCamera->setState(cs);
  }

  auto &camera        = frame.child("camera");
  if (camera.hasChild("aspect"))
    camera["aspect"] = optImageSize.x / (float)optImageSize.y;
  camera["position"]  = arcballCamera->eyePos();
  camera["direction"] = arcballCamera->lookDir();
  camera["up"]        = arcballCamera->upDir();

  if (cmdlCam) {
    camera["position"]   = pos;
    camera["direction"]  = normalize(gaze - pos);
    camera["up"]         = up;
  }

  frame["world"].createChild("light", "ambient");

  frame.render();
  // Accumulate several frames
  for (auto i = 0; i < optSPP - 1; i++) {
    frame.immediatelyWait = true;
    frame.startNewFrame();
  }

  // Only denoise the final frame
  // XXX TODO if optDenoiser == 2, save both the noisy and denoised color
  // buffers.  How best to do that since the frame op will alter the final
  // buffer?
  if (denoiserAvailable && optDenoiser)
    frame["denoiseFB"] = true;
  frame.immediatelyWait = true;
  frame.startNewFrame();

  auto size          = frame["framebuffer"]["size"].valueAs<vec2i>();
  const void *pixels = frame.mapFrame();

  auto colorFormatStr =
      frame["framebuffer"]["colorFormat"].valueAs<std::string>();

  if (colorFormatStr == "float") {
    optImageName += ".pfm";
    rkcommon::utility::writePFM(optImageName, size.x, size.y, (vec4f *)pixels);
  } else {
    optImageName += ".ppm";
    rkcommon::utility::writePPM(
        optImageName, size.x, size.y, (uint32_t *)pixels);
  }

  frame.unmapFrame((void *)pixels);

#if 0  // XXX for debug
  frame.traverse<sg::PrintNodes>();
#endif

  std::cout << "\nresult saved to '" << optImageName << "'\n";
}

void BatchContext::importFiles()
{
  importedModels = createNode("importXfm", "transform", affine3f{one});

  for (auto file : filesToImport) {
    try {
      rkcommon::FileName fileName(file);
      std::string nodeName = fileName.base() + "_importer";

      std::cout << "Importing: " << file << std::endl;
      auto oldRegistrySize = baseMaterialRegistry->children().size();
      auto importer        = sg::getImporter(file);
      if (importer != "") {
        auto &imp =
            importedModels->createChildAs<sg::Importer>(nodeName, importer);
        imp["file"] = std::string(file);
        imp.importScene(baseMaterialRegistry);

        if (oldRegistrySize != baseMaterialRegistry->children().size()) {
          auto newMats =
              baseMaterialRegistry->children().size() - oldRegistrySize;
          std::cout << "Importer added " << newMats << " material(s)"
                    << std::endl;
        }
      }
    } catch (...) {
      std::cerr << "Failed to open file '" << file << "'!\n";
    }
  }
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
   -r     --renderer [type] (default "scivis")
            rendererType scivis or pathtracer
   -c     --camera [type] (default "perspective")
            cameraType perspective or panoramic
   -vp    [x y z] camera position  
   -vu    [x y z] camera up  
   -vi    [x y z] camera look-at  
   -g     --grid [x y z] (default 1 1 1, single instance)
            instace a grid of models)text"
            << std::endl;
  if (denoiserAvailable) {
    std::cout <<
        R"text(
   -oidn  --denoiser [0,1,2] (default 0)
            image denoiser (0 = off, 1 = on, 2 = save both)
)text" << std::endl;
  }
}
