// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../../Node.h"

namespace ospray {
  namespace sg {

  struct OSPSG_INTERFACE Geometry
      : public OSPNode<cpp::Geometry, NodeType::GEOMETRY>
  {
    Geometry(const std::string &osp_type);
    ~Geometry() override = default;
  };

  }  // namespace sg
} // namespace ospray
