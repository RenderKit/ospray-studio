// Copyright 2009-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

// sg
#include "sg/Node.h"

namespace ospray {
  namespace sg {

  struct OSPSG_INTERFACE Texture : public OSPNode<cpp::Texture, NodeType::TEXTURE>
  {
    Texture(const std::string &type);
    virtual ~Texture() override = default;

    NodeType type() const override;
  };

  }  // namespace sg
} // namespace ospray
