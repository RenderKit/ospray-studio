// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../Node.h"
#include "sg/renderer/MaterialRegistry.h"

namespace ospray {
  namespace sg {

  struct OSPSG_INTERFACE Generator : public Node
  {
    Generator();
    virtual ~Generator() = default;

    NodeType type() const override;

    virtual void generateData();

    virtual void generateDataAndMat(std::shared_ptr<sg::MaterialRegistry> materialRegistry);

  };

  }  // namespace sg
} // namespace ospray
