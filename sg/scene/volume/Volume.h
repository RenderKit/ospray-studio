// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../../Node.h"

namespace ospray {
  namespace sg {

  struct OSPSG_INTERFACE Volume : public OSPNode<cpp::Volume, NodeType::VOLUME>
  {
    Volume(const std::string &osp_type);
    ~Volume() override = default;

    NodeType type() const override;
  };

  }  // namespace sg
} // namespace ospray
