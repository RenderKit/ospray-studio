// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Volume.h"

namespace ospray {
  namespace sg {

  Volume::Volume(const std::string &osp_type)
  {
    setValue(cpp::Volume(osp_type));
    createChild("visible", "bool", true);
    createChild("filter", "int", "0 = nearest, 100 = trilinear", 0);
  }

  NodeType Volume::type() const
  {
    return NodeType::VOLUME;
  }

  void Volume::load(const FileName &fileNameAbs)
  {
    auto &dimensions = child("dimensions").valueAs<vec3i>();

    if (dimensions.x <= 0 || dimensions.y <= 0 || dimensions.z <= 0) {
      throw std::runtime_error(
          "invalid volume dimensions");
    }

    if (!fileLoaded) {
      auto &voxelType = child("voxelType").valueAs<int>();
      FileName realFileName = fileNameAbs;
      FILE *file = fopen(realFileName.c_str(), "r");

      if (!file) {
        throw std::runtime_error(
            "Volume::load : could not open file '"
            + realFileName.str());
      }
      OSPDataType voxelDataType = (OSPDataType)voxelType;
      const size_t voxelSize = sizeOf(voxelDataType);
      const size_t nVoxels =
          (size_t)dimensions.x * (size_t)dimensions.y * (size_t)dimensions.z;

      switch (voxelDataType) {
      case OSP_UCHAR: {
        std::vector<unsigned char> voxels(dimensions.long_product());
        if (fread(voxels.data(), voxelSize, nVoxels, file) != nVoxels) {
          throw std::runtime_error(
              "read incomplete data (truncated file or "
              "wrong format?!)");
        }
        createChildData("data", dimensions, 0, voxels.data());
        break;
      }

      case OSP_SHORT: {
        std::vector<int16_t> voxels(dimensions.long_product());
        if (fread(voxels.data(), voxelSize, nVoxels, file) != nVoxels) {
          throw std::runtime_error(
              "read incomplete data (truncated file or "
              "wrong format?!)");
        }
        createChildData("data", dimensions, 0, voxels.data());
        break;
      }

      case OSP_USHORT: {
        std::vector<uint16_t> voxels(dimensions.long_product());
        if (fread(voxels.data(), voxelSize, nVoxels, file) != nVoxels) {
          throw std::runtime_error(
              "read incomplete data (truncated file or "
              "wrong format?!)");
        }
        createChildData("data", dimensions, 0, voxels.data());
        break;
      }

      case OSP_INT: {
        std::vector<int> voxels(dimensions.long_product());
        if (fread(voxels.data(), voxelSize, nVoxels, file) != nVoxels) {
          throw std::runtime_error(
              "read incomplete data (truncated file or "
              "wrong format?!)");
        }
        createChildData("data", dimensions, 0, voxels.data());
        break;
      }

      case OSP_FLOAT: {
        std::vector<float> voxels(dimensions.long_product());
        if (fread(voxels.data(), voxelSize, nVoxels, file) != nVoxels) {
          throw std::runtime_error(
              "read incomplete data (truncated file or "
              "wrong format?!)");
        }
        createChildData("data", dimensions, 0, voxels.data());
        break;
      }

      case OSP_DOUBLE: {
        std::vector<double> voxels(dimensions.long_product());
        if (fread(voxels.data(), voxelSize, nVoxels, file) != nVoxels) {
          throw std::runtime_error(
              "read incomplete data (truncated file or "
              "wrong format?!)");
        }
        createChildData("data", dimensions, 0, voxels.data());
        break;
      }

      default:
        throw std::runtime_error(
            "sg::extendVoxelRange: unsupported voxel type!");
      }

      fclose(file);
      fileLoaded = true;

      // handle isosurfaces too
    }
  }

  }  // namespace sg
} // namespace ospray
