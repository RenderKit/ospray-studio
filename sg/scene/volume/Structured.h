// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

// sg
#include "../../Data.h"
#include "Volume.h"
// rkcommon
#include "rkcommon/os/FileName.h"

namespace ospray {
  namespace sg {

  struct OSPSG_INTERFACE StructuredVolume : public Volume
  {
    StructuredVolume();
    virtual ~StructuredVolume() override = default;
    void load(const FileName &fileName);

   private:
    bool fileLoaded{false};
  };

  }  // namespace sg
} // namespace ospray
