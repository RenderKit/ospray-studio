// ======================================================================== //
// Copyright 2020 Intel Corporation                                         //
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

#pragma once

// sg
#include "../../Data.h"
#include "Volume.h"
// rkcommon
#include "rkcommon/os/FileName.h"

#if USE_OPENVDB
#include <openvdb/openvdb.h>
#endif //USE_OPENVDB

namespace ospray::sg {

  struct OSPSG_INTERFACE VdbVolume : public Volume
  {
    VdbVolume();
    virtual ~VdbVolume() override = default;
    void load(const FileName &fileName);

   private:
#if USE_OPENVDB
    bool fileLoaded{false};
    openvdb::GridBase::Ptr grid{nullptr};
    std::vector<float> tiles;
#endif //USE_OPENVDB
  };

}  // namespace ospray::sg
