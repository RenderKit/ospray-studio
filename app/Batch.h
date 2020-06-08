// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ArcballCamera.h"
// ospray sg
#include "sg/Frame.h"
#include "sg/Node.h"
#include "sg/renderer/MaterialRegistry.h"

using namespace ospcommon::math;
using namespace ospray;
using namespace ospray::sg;
using ospcommon::make_unique;

class BatchContext
{
 public:
  BatchContext(const vec2i &imageSize);
  ~BatchContext() {}

  bool parseCommandLine(int &ac, const char **&av);
  void importFiles();
  void render();

 protected:
  std::shared_ptr<sg::Frame> frame_ptr;
  std::shared_ptr<sg::MaterialRegistry> baseMaterialRegistry;
  std::vector<std::string> filesToImport;
  NodePtr importedModels;

  std::string optRendererTypeStr = "scivis";
  std::string optImageName       = "ospBatch";
  vec2i optImageSize             = (1024, 768);
  int optSPP                     = 32;
  bool optGridEnable             = false;
  vec3i optGridSize              = (1, 1, 1);

  // Arcball camera instance
  std::unique_ptr<ArcballCamera> arcballCamera;

  void printHelp();
};
