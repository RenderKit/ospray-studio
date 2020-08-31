// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

// sg
#include "../../Data.h"
#include "Volume.h"
// ospcommon
#include "rkcommon/os/FileName.h"

namespace ospray {
  namespace sg {

  struct OSPSG_INTERFACE StructuredSpherical : public Volume
  {
    StructuredSpherical();
    virtual ~StructuredSpherical() override = default;
    void load(const FileName &fileName);

   private:
    bool fileLoaded{false};
  };

  }  // namespace sg
} // namespace ospray
