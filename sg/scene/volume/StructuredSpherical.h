// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

// sg
#include "../../Data.h"
#include "Volume.h"

namespace ospray {
  namespace sg {

  struct OSPSG_INTERFACE StructuredSpherical : public Volume
  {
    StructuredSpherical();
    virtual ~StructuredSpherical() override = default;
  };

  // StructuredSpherical definitions /////////////////////////////////////////////

  StructuredSpherical::StructuredSpherical() : Volume("structuredSpherical") {}

  OSP_REGISTER_SG_NODE_NAME(StructuredSpherical, structuredSpherical);

  }  // namespace sg
} // namespace ospray
