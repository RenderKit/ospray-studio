// Copyright 2018 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <fstream>
#include <vector>
#include "rkcommon/math/vec.h"
#include "../../Node.h"

using namespace rkcommon::math;

namespace ospray {
  namespace sg{

    struct OSPSG_INTERFACE RawFileStructuredVolume
    {
      RawFileStructuredVolume(const std::string &filename,
                              const vec3i &dimensions);

      std::vector<float> generateVoxels();

     protected:
      std::string filename;
      vec3i dimensions;
    };

}  // namespace sg
} // namespace ospray

