// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

// sg
#include "../../Data.h"
#include "Volume.h"

namespace ospray {
  namespace sg {

  struct OSPSG_INTERFACE StructuredVolume : public Volume
  {
    StructuredVolume();
    virtual ~StructuredVolume() override = default;
  };

  // StructuredVolume definitions /////////////////////////////////////////////

  StructuredVolume::StructuredVolume() : Volume("structuredRegular") {}

  OSP_REGISTER_SG_NODE_NAME(StructuredVolume, structuredRegular);

  }  // namespace sg
} // namespace ospray
