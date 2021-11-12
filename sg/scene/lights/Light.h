// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../../Node.h"

namespace ospray {
  namespace sg {

  struct OSPSG_INTERFACE Light : public OSPNode<cpp::Light, NodeType::LIGHT>
  {
    Light(std::string type);
    ~Light() override = default;
    NodeType type() const override;
  };

  }  // namespace sg
} // namespace ospray
