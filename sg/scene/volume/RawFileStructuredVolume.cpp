// Copyright 2018-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "RawFileStructuredVolume.h"

namespace ospray {
  namespace sg {

  RawFileStructuredVolume::RawFileStructuredVolume(const std::string &filename,
                                                   const vec3i &dimensions)
      : filename(filename), dimensions(dimensions)
      {
  }

  std::vector<float> RawFileStructuredVolume::generateVoxels()
  {
    std::cout << "suing raw file structured volume" << std::endl;
    std::vector<float> voxels(dimensions.product());

    std::ifstream input(filename, std::ios::binary);

    if (!input) {
      throw std::runtime_error("error opening raw volume file");
    }

    input.read((char *)voxels.data(), dimensions.product() * sizeof(float));

    if (!input.good()) {
      throw std::runtime_error("error reading raw volume file");
    }

    return voxels;
  }
  }  // namespace sg
} // namespace ospray
