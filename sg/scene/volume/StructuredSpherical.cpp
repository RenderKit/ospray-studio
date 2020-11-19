// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Volume.h"
#include "StructuredSpherical.h"

namespace ospray {
  namespace sg {

  // StructuredSpherical definitions /////////////////////////////////////////////

  StructuredSpherical::StructuredSpherical() : Volume("structuredSpherical") {}

    ///! \brief file name of the xml doc when the node was loaded from xml
  /*! \detailed we need this to properly resolve relative file names */
  void StructuredSpherical::load(const FileName &fileNameAbs)
  {
    auto &dimensions = child("dimensions").valueAs<vec3i>();
    if (dimensions.x <= 0 || dimensions.y <= 0 || dimensions.z <= 0) {
      throw std::runtime_error(
          "StructuredRegular::render(): "
          "invalid volume dimensions");
    }

    std::vector<float> voxels(dimensions.product());

    std::ifstream input(fileNameAbs.c_str(), std::ios::binary);

    if (!input) {
      throw std::runtime_error("error opening raw volume file");
    }

    input.read((char *)voxels.data(), dimensions.product() * sizeof(float));

    if (!input.good()) {
      throw std::runtime_error("error reading raw volume file");
    }
     
    createChildData("data", dimensions, OSP_FLOAT, voxels.data());

    fileLoaded = true;
  }

   OSP_REGISTER_SG_NODE_NAME(StructuredSpherical, structuredSpherical);

  }  // namespace sg
} // namespace ospray
