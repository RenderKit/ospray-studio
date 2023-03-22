// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Generator.h"
#include "WaveletField.h"
#include "sg/Mpi.h"

// std
#include <random>
// rkcommon
#include "rkcommon/tasking/parallel_for.h"

namespace ospray {
namespace sg {

struct WaveletVolume : public Generator
{
  WaveletVolume();
  ~WaveletVolume() override = default;

  void generateData() override;
};

OSP_REGISTER_SG_NODE_NAME(WaveletVolume, generator_wavelet);

// WaveletVolume definitions ////////////////////////////////////////////////

WaveletVolume::WaveletVolume()
{
  auto &parameters = child("parameters");

  parameters.createChild("dimensions", "vec3i", vec3i(256));
  parameters.createChild("gridOrigin", "vec3f", vec3f(-1.f));
  parameters.createChild("gridSpacing", "vec3f", vec3f(2.f / 100));

  auto &xfm = createChild("xfm", "transform");
}

void WaveletVolume::generateData()
{
  auto &xfm = child("xfm");
  auto &tf = xfm.createChild("transferFunction", "transfer_function_turbo");

  // Create voxel data

  auto &parameters = child("parameters");

  auto dimensions = parameters["dimensions"].valueAs<vec3i>();
  auto gridOrigin = parameters["gridOrigin"].valueAs<vec3f>();
  auto gridSpacing = parameters["gridSpacing"].valueAs<vec3f>();
  std::vector<float> voxels;

  // Create sg subtree
  auto &volume = tf.createChild("wavelet", "structuredRegular");

  if (sgUsingMpi())
  {
    //divide up world space by number of MPI ranks and set centers according to local rank
    const vec3i grid = compute_grid(sgMpiWorldSize());
    const vec3i brickId(sgMpiRank() % grid.x, (sgMpiRank() / grid.x) % grid.y, sgMpiRank() / (grid.x * grid.y));
    vec3i brick_dims = dimensions / grid;
    const vec3i brick_lower = brickId * brick_dims;
    const vec3i brick_upper = brickId * brick_dims + brick_dims;

    box3f brick_bounds = box3f(gridOrigin + vec3f(brick_lower) * gridSpacing, gridOrigin + vec3f(brick_upper) * gridSpacing);
    vec3i brick_ghostDims = brick_dims;
    vec3i brickOffset = brick_lower;
    box3f brick_ghostBounds = brick_bounds;
    {
        const auto ghost_faces = compute_ghost_faces(brickId, grid);
        for (size_t i = 0; i < 3; ++i) {
            if (ghost_faces[i] & NEG_FACE) {
                brick_ghostDims[i] += 1;
                brick_ghostBounds.lower[i] -= gridSpacing[i];
                brickOffset[i] -= 1;
            }
            if (ghost_faces[i] & POS_FACE) {
                brick_ghostDims[i] += 1;
                brick_ghostBounds.upper[i] += gridSpacing[i];
            }
        }
    }

    vec3i brick_ghostDims_plus1 = brick_ghostDims + vec3i(1,1,1);
    voxels = std::vector<float>(brick_ghostDims_plus1.long_product());

    auto transformLocalToObject = [&](const vec3f &localCoordinates) {
      return brick_ghostBounds.lower + localCoordinates * gridSpacing;
    };

    const float debugScale = 1.0;//(sgMpiRank()+1) / float(sgMpiWorldSize()); //helps show MPIness

    tasking::parallel_for(brick_ghostDims.z, [&](int z) {
      for (size_t y = 0; y < (size_t)brick_ghostDims.y; y++) {
        for (size_t x = 0; x < (size_t)brick_ghostDims.x; x++) {
          size_t index =
              z * (size_t)brick_ghostDims.y * brick_ghostDims.x + y * brick_ghostDims.x + x;
          vec3f objectCoordinates = transformLocalToObject(vec3f(x, y, z));
          voxels[index] = getWaveletValue(objectCoordinates) * debugScale;
        }
      }
    });

    volume.createChild("voxelType") = int(OSP_FLOAT);
    volume.createChild("gridOrigin", "vec3f", brick_ghostBounds.lower);
    volume.createChild("gridSpacing", "vec3f", gridSpacing);
    volume.createChildData("data", brick_ghostDims, 0, voxels.data());

    volume.createChild("mpiRegion") = brick_bounds;
    volume.child("mpiRegion").setSGNoUI();
    volume.child("mpiRegion").setSGOnly();
  }
  else
  {
    //serial code
    voxels = std::vector<float>(dimensions.long_product());

    auto transformLocalToObject = [&](const vec3f &localCoordinates) {
      return gridOrigin + localCoordinates * gridSpacing;
    };

    tasking::parallel_for(dimensions.z, [&](int z) {
      for (size_t y = 0; y < (size_t)dimensions.y; y++) {
        for (size_t x = 0; x < (size_t)dimensions.x; x++) {
          size_t index =
              z * (size_t)dimensions.y * dimensions.x + y * dimensions.x + x;
          vec3f objectCoordinates = transformLocalToObject(vec3f(x, y, z));
          voxels[index] = getWaveletValue(objectCoordinates);
        }
      }
    });

    volume.createChild("voxelType") = int(OSP_FLOAT);
    volume.createChild("gridOrigin", "vec3f", gridOrigin);
    volume.createChild("gridSpacing", "vec3f", gridSpacing);
    volume.createChildData("data", dimensions, 0, voxels.data());

  }

  const auto minmax = std::minmax_element(begin(voxels), end(voxels));
  range1f valueRange = range1f(*std::get<0>(minmax), *std::get<1>(minmax));

#ifdef USE_MPI
  range1f localValueRange = valueRange;
  if (sgUsingMpi()){
      MPI_Allreduce(
          &localValueRange.lower, &valueRange.lower, 1, MPI_FLOAT, MPI_MIN, MPI_COMM_WORLD);
      MPI_Allreduce(
          &localValueRange.upper, &valueRange.upper, 1, MPI_FLOAT, MPI_MAX, MPI_COMM_WORLD);
  }
#endif

  volume["value"] = valueRange;
  // Although cell values lie outside (0, 1), leave the default transfer
  // function range.  It's an interesting image.
  tf["value"] = range1f(0.f, 1.f);

}

} // namespace sg
} // namespace ospray
