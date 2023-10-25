// Copyright 2018 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include <array>
#include <memory>
#include <random>
#include <vector>
#include "../../Data.h"
#include "../../Node.h"
#include "RawFileStructuredVolume.h"
#include "Volume.h"
#include "Structured.h"
#include "rkcommon/math/vec.h"
#include "rkcommon/tasking/AsyncTask.h"

#include "rkcommon/tasking/parallel_for.h"

using namespace rkcommon;
using namespace rkcommon::math;

// the following struct implements one Volume time step for the time series.

namespace ospray {
  namespace sg {

  struct VolumeTimestep
  {
    VolumeTimestep(const std::string &filename,
                   const OSPDataType &voxelType,
                   const vec3i &dimensions,
                   const vec3f &gridOrigin,
                   const vec3f &gridSpacing)
        : filename(filename),
          voxelType(voxelType),
          dimensions(dimensions),
          gridOrigin(gridOrigin),
          gridSpacing(gridSpacing)
    {
    }

    void queueGenerateVolumeData()
    {
      if (localLoading) {
        return;
      }
      if (sgVolume) {
        return;
      }

      if (!generateVolumeDataTask) {
        generateVolumeDataTask =
            std::shared_ptr<tasking::AsyncTask<std::vector<float>>>(
                new tasking::AsyncTask<std::vector<float>>([=]() {
                  std::shared_ptr<ospray::sg::RawFileStructuredVolume> temp(
                      new ospray::sg::RawFileStructuredVolume(filename,
                                                              dimensions));

                  return temp->generateVoxels();
                }));
      }
    }

    void waitGenerateVolumeData()
    {
      if (localLoading) {
        return;
      }
      if (sgVolume) {
        return;
      }

      if (!generateVolumeDataTask) {
        queueGenerateVolumeData();
      }

      generateVolumeDataTask->wait();
    }

    std::shared_ptr<sg::Volume> createSGVolume()
    {
      if (!sgVolume) {
        if (!generateVolumeDataTask) {
          queueGenerateVolumeData();
        }

        sgVolume = std::static_pointer_cast<sg::Volume>(
            createNode("sgVolume_" + to_string(variableNum), "structuredRegular"));

        if (!localLoading) {
          std::vector<float> voxels(dimensions.long_product());
          voxels = generateVolumeDataTask->get();
          generateVolumeDataTask.reset();
          sgVolume->createChildData("data", dimensions, 0, voxels.data());
        } else {
          sgVolume->nodeAs<sg::StructuredVolume>()->load(filename);
        }

        sgVolume->createChild("dimensions", "vec3i", dimensions);
        sgVolume->createChild("gridOrigin", "vec3f", gridOrigin);
        sgVolume->createChild("gridSpacing", "vec3f", gridSpacing);
        sgVolume->createChild("voxelType", "OSPDataType", voxelType);

        fileLoaded = true;
      }

      return sgVolume;
    }

    std::string filename;
    OSPDataType voxelType;
    vec3i dimensions;
    vec3f gridOrigin;
    vec3f gridSpacing;

    bool fileLoaded{false};
    bool localLoading{false};
    int variableNum{0};

    std::shared_ptr<tasking::AsyncTask<std::vector<float>>> generateVolumeDataTask;

    std::shared_ptr<sg::Volume> sgVolume;
  };

  }  // namespace sg
} // namespace ospray
