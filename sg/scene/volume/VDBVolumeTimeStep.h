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
#include "rkcommon/math/vec.h"
#include "rkcommon/tasking/AsyncTask.h"

#include "rkcommon/tasking/parallel_for.h"
#include "Vdb.h"


using namespace rkcommon;
using namespace rkcommon::math;

// the following struct implements one Volume time step for the time series.

namespace ospray {
  namespace sg {

  struct VDBVolumeTimestep
  {
    VDBVolumeTimestep(const std::string &filename) : fs(filename)
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
          generateVolumeDataTask = std::shared_ptr<tasking::AsyncTask<VDBData>>(
              new rkcommon::tasking::AsyncTask<VDBData>([=]() {
                // Load remaining leaves, but use the usage buffer as guidance.
               std::shared_ptr<ospray::sg::VdbVolume> temp(
                      new ospray::sg::VdbVolume());

                  return temp->generateVDBData(fs);
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
#if USE_OPENVDB
      if (!sgVolume) {
        if (!localLoading && !generateVolumeDataTask) {
          queueGenerateVolumeData();
        }

        sgVolume = std::static_pointer_cast<sg::Volume>(
            createNode("sgVolume_" + to_string(variableNum), "volume_vdb"));

        if (!localLoading) {
          auto vdbData = generateVolumeDataTask->get();
          generateVolumeDataTask.reset();
          sgVolume->createChildData("node.level", vdbData.level);
          sgVolume->createChildData("node.origin", vdbData.origin);
          sgVolume->createChildData("node.data", vdbData.data);
          sgVolume->createChildData("indexToObject", vdbData.bufI2o);
        } else {
          sgVolume->nodeAs<sg::VdbVolume>()->load(fs);
        }
        fileLoaded = true;
        sgVolume->createChild("densityScale", "float");
        sgVolume->createChild("anisotropy", "float");
      }
#else
      throw std::runtime_error(
          "OpenVDB not enabled in build.  Rebuild Studio, selecting "
          "ENABLE_OPENVDB in cmake.");
      sgVolume = nullptr;
#endif
      return sgVolume;
    }

    std::string fs;
    int variableNum{0};
    bool localLoading{false};
    bool fileLoaded{false};
    std::shared_ptr<sg::Volume> sgVolume;
    std::shared_ptr<rkcommon::tasking::AsyncTask<VDBData>> generateVolumeDataTask;

  };

  }  // namespace sg
} // namespace ospray
