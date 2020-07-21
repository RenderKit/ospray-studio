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
#include "Vdb.h"


using namespace rkcommon;
using namespace rkcommon::math;

// the following struct implements one Volume time step for the time series.

bool g_lowResMode   = false;
bool g_localLoading = false;

namespace ospray::sg {

  struct VDBVolumeTimestep
  {
    VDBVolumeTimestep(const std::string &filename) : fs(filename)
    {
    }

    std::shared_ptr<sg::Volume> createSGVolume()
    {
    if (fs.length() > 4 && fs.substr(fs.length()-4) == ".vdb")
    {
      auto vol = std::static_pointer_cast<sg::VdbVolume>(
          sg::createNode("volume", "volume_vdb"));
      vol->load(fs);
      volumeImport = vol;
      return volumeImport;
    }
    }

    std::string fs;

    std::shared_ptr<sg::Volume> volumeImport;
  };

}  // namespace ospray::sg