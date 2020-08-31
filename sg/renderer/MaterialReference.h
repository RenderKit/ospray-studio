// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../Node.h"

namespace ospray {
  namespace sg {

  struct OSPSG_INTERFACE MaterialReference : public Node
  {
    MaterialReference();
    virtual ~MaterialReference() override = default;

    NodeType type() const override;
  };

  }  // namespace sg
} // namespace ospray
