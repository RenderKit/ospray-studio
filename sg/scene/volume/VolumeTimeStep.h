// ======================================================================== //
// Copyright 2018 Intel Corporation                                         //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#include <array>
#include <memory>
#include <random>
#include <vector>
#include "../../Data.h"
#include "../../Node.h"
#include "RawFileStructuredVolume.h"
#include "Volume.h"
#include "rkcommon/math/vec.h"
#include "rkcommon/tasking/AsyncTask.h"

#include "rkcommon/tasking/parallel_for.h"

using namespace rkcommon;
using namespace rkcommon::math;

// the following struct implements one Volume time step for the time series.

namespace ospray::sg {

  struct VolumeTimestep
  {
    VolumeTimestep(const std::string &filename,
                   const int &voxelType,
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

    void queueGenerateVoxels()
    {
      if (localLoading) {
        return;
      }

      if (sgVolume) {
        return;
      }

      if (!generateVoxelsTask) {
        generateVoxelsTask =
            std::shared_ptr<tasking::AsyncTask<std::vector<float>>>(
                new tasking::AsyncTask<std::vector<float>>([=]() {
                  std::shared_ptr<ospray::sg::RawFileStructuredVolume> temp(
                      new ospray::sg::RawFileStructuredVolume(filename,
                                                              dimensions));

                  return temp->generateVoxels();
                }));
      }
    }

    void waitGenerateVoxels()
    {
      if (localLoading) {
        return;
      }

      if (sgVolume) {
        return;
      }

      if (!generateVoxelsTask) {
        queueGenerateVoxels();
      }

      generateVoxelsTask->wait();
    }

    std::shared_ptr<sg::Volume> createSGVolume()
    {
      if (!sgVolume) {
        if (!localLoading && !generateVoxelsTask) {
          queueGenerateVoxels();
        }

        sgVolume = std::static_pointer_cast<sg::Volume>(
            createNode("sgVolume", "structuredRegular"));

        if (!localLoading) {
          std::vector<float> voxels(dimensions.long_product());
          voxels = generateVoxelsTask->get();
          generateVoxelsTask.reset();
          sgVolume->createChildData("data", dimensions, 0, voxels.data());
        } else {
          // address filename param on sg::Volume
          sgVolume->createChild("filename", "string", filename.c_str());
        }

        sgVolume->createChild("dimensions", "vec3i", dimensions);
        sgVolume->createChild("gridOrigin", "vec3f", gridOrigin);
        sgVolume->createChild("gridSpacing", "vec3f", gridSpacing);
        sgVolume->createChild("voxelType", "int", voxelType);
      }

      return sgVolume;
    }

    std::string filename;
    int voxelType;
    vec3i dimensions;
    vec3f gridOrigin;
    vec3f gridSpacing;

    bool fileLoaded{false};
    bool localLoading = false;

    std::shared_ptr<tasking::AsyncTask<std::vector<float>>> generateVoxelsTask;

    std::shared_ptr<sg::Volume> sgVolume;
  };

}  // namespace ospray::sg