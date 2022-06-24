// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

// sg
#include "../../Data.h"
#include "Volume.h"

#if USE_OPENVDB
#include <openvdb/openvdb.h>
#endif //USE_OPENVDB

namespace ospray {
  namespace sg {

    struct VDBData{
      std::vector<uint32_t> level;
      std::vector<vec3i> origin;
      std::vector<float> data;
      std::vector<float> bufI2o;
      std::vector<uint32_t> format;
    };

  struct OSPSG_INTERFACE VdbVolume : public Volume
  {
    VdbVolume();
    virtual ~VdbVolume() override = default;
    void load(const FileName &fileName) override;
    VDBData generateVDBData(const FileName &fileNameAbs);

   private:
#if USE_OPENVDB
    bool fileLoaded{false};
    openvdb::GridBase::Ptr grid{nullptr};
    std::vector<float> tiles;

#endif //USE_OPENVDB
  };

  }  // namespace sg
} // namespace ospray
