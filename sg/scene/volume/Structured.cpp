// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Structured.h"
#include <sstream>
#include <string>

namespace ospray {
  namespace sg {

  /*! helper function to help build voxel ranges during parsing */
  template <typename T>
  inline void extendVoxelRange(rkcommon::math::vec2f &voxelRange,
                               const T *voxel,
                               size_t num)
  {
    for (size_t i = 0; i < num; ++i) {
      if (!std::isnan(static_cast<float>(voxel[i]))) {
        voxelRange.x = std::min(voxelRange.x, static_cast<float>(voxel[i]));
        voxelRange.y = std::max(voxelRange.y, static_cast<float>(voxel[i]));
      }
    }
  }

  //! Convenient wrapper that will do the template dispatch for you based on
  //  the voxelType passed
  inline void extendVoxelRange(rkcommon::math::vec2f &voxelRange,
                               const OSPDataType voxelType,
                               const unsigned char *voxels,
                               const size_t numVoxels)
  {
    switch (voxelType) {
    case OSP_UCHAR:
      extendVoxelRange(voxelRange, voxels, numVoxels);
      break;
    case OSP_SHORT:
      extendVoxelRange(
          voxelRange, reinterpret_cast<const short *>(voxels), numVoxels);
      break;
    case OSP_USHORT:
      extendVoxelRange(voxelRange,
                       reinterpret_cast<const unsigned short *>(voxels),
                       numVoxels);
      break;
    case OSP_FLOAT:
      extendVoxelRange(
          voxelRange, reinterpret_cast<const float *>(voxels), numVoxels);
      break;
    case OSP_DOUBLE:
      extendVoxelRange(
          voxelRange, reinterpret_cast<const double *>(voxels), numVoxels);
      break;
    default:
      throw std::runtime_error("sg::extendVoxelRange: unsupported voxel type!");
    }
  }

  bool unsupportedVoxelType(const std::string &type)
  {
    return type != "uchar" && type != "ushort" && type != "short" &&
           type != "float" && type != "double";
  }

  // StructuredVolume definitions /////////////////////////////////////////////

  StructuredVolume::StructuredVolume() : Volume("structuredRegular")
  {
    createChild("voxelType", "int");
    createChild("gridOrigin", "vec3f");
    createChild("gridSpacing", "vec3f");
    createChild("dimensions", "vec3i");
  }

  ///! \brief file name of the xml doc when the node was loaded from xml
  /*! \detailed we need this to properly resolve relative file names */
  void StructuredVolume::load(const FileName &fileNameAbs)
  {
    // the following hard coded volume properties should exist either in volume file headers or,
    // xml kind supporting docs with binary volume data
    createChild("voxelType", "int", int(OSP_FLOAT));
    createChild("dimensions", "vec3i", vec3i(18, 25, 18));
    createChild("gridOrigin", "vec3f", vec3f(-1.f));
    createChild("gridSpacing", "vec3f", vec3f(2.f / 100));

    auto &dimensions = child("dimensions").valueAs<vec3i>();
    std::vector<float> voxels(dimensions.long_product());

    if (dimensions.x <= 0 || dimensions.y <= 0 || dimensions.z <= 0) {
      throw std::runtime_error(
          "StructuredRegular::render(): "
          "invalid volume dimensions");
    }

    if (!fileLoaded) {
      auto &voxelType          = child("voxelType").valueAs<int>();
      FileName realFileName    = fileNameAbs;
      FILE *file = fopen(realFileName.c_str(), "r");

      if (!file) {
        throw std::runtime_error(
            "StructuredVolume::load : could not open file '" +
            realFileName.str());
      }

      const size_t voxelSize = sizeof(voxelType);
      const size_t nVoxels =
          (size_t)dimensions.x * (size_t)dimensions.y * (size_t)dimensions.z;

      if (fread(voxels.data(), voxelSize, nVoxels, file) != nVoxels) {
        throw std::runtime_error(
            "read incomplete data (truncated file or "
            "wrong format?!)");
      }

      createChildData("data", dimensions, 0, voxels.data());
      fclose(file);
      fileLoaded = true;

      // handle isosurfaces too
    }
  }

  OSP_REGISTER_SG_NODE_NAME(StructuredVolume, structuredRegular);

  }  // namespace sg
} // namespace ospray
