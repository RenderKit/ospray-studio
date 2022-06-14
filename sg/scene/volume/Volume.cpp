// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Volume.h"

namespace ospray {
namespace sg {

Volume::Volume(const std::string &osp_type)
{
  setValue(cpp::Volume(osp_type));
  createChild("visible", "bool", true);
  createChild("filter", "int", "0 = nearest, 100 = trilinear", 0);

  createChild("densityScale",
      "float",
      "makes volumes uniformly thinner or thicker\n"
      "(value range is unbounded, slider is bounded for convenience.\n"
      "shift-LMB to enter larger values manually)",
      1.f)
      .setMinMax(0.f, 100.f);
  createChild("anisotropy",
      "float",
      "anisotropy of the (Henyey-Greenstein) phase function in [-1–1]\n"
      "(path tracer only), default to isotropic scattering",
      0.f)
      .setMinMax(-1.f, 1.f);

  // All volumes track their valueRange
  createChild("valueRange", "range1f", range1f(0.f, 1.f));
  child("valueRange").setSGOnly();
  child("valueRange").setReadOnly();
}

NodeType Volume::type() const
{
  return NodeType::VOLUME;
}

template <typename T>
void Volume::loadVoxels(FILE *file, const vec3i dimensions)
{
  const size_t nVoxels = dimensions.long_product();
  std::vector<T> voxels(nVoxels);

  if (fread(voxels.data(), sizeof(T), nVoxels, file) != nVoxels) {
    throw std::runtime_error(
        "read incomplete data (truncated file or wrong format?!)");
  }
  const auto minmax = std::minmax_element(begin(voxels), end(voxels));
  child("valueRange") = range1f(*std::get<0>(minmax), *std::get<1>(minmax));

  createChildData("data", dimensions, 0, voxels.data());
}

void Volume::load(const FileName &fileNameAbs)
{
  auto &dimensions = child("dimensions").valueAs<vec3i>();

  if (dimensions.x <= 0 || dimensions.y <= 0 || dimensions.z <= 0) {
    throw std::runtime_error("invalid volume dimensions");
  }

  if (!fileLoaded) {
    auto &voxelType = child("voxelType").valueAs<int>();
    FileName realFileName = fileNameAbs;
    FILE *file = fopen(realFileName.c_str(), "r");

    if (!file) {
      throw std::runtime_error(
          "Volume::load : could not open file '" + realFileName.str());
    }
    OSPDataType voxelDataType = (OSPDataType)voxelType;

    switch (voxelDataType) {
    case OSP_UCHAR:
      loadVoxels<unsigned char>(file, dimensions);
      break;
    case OSP_SHORT:
      loadVoxels<int16_t>(file, dimensions);
      break;
    case OSP_USHORT:
      loadVoxels<uint16_t>(file, dimensions);
      break;
    case OSP_INT:
      loadVoxels<int>(file, dimensions);
      break;
    case OSP_FLOAT:
      loadVoxels<float>(file, dimensions);
      break;
    case OSP_DOUBLE:
      loadVoxels<double>(file, dimensions);
      break;
    default:
      throw std::runtime_error("sg::extendVoxelRange: unsupported voxel type!");
    }

    fclose(file);
    fileLoaded = true;

    // handle isosurfaces too
  }
}

} // namespace sg
} // namespace ospray
