// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Generator.h"

namespace ospray {
  namespace sg {

  Generator::Generator()
  {
    createChild("parameters");
  }

  NodeType Generator::type() const
  {
    return NodeType::GENERATOR;
  }

  void Generator::generateDataAndMat(std::shared_ptr<sg::MaterialRegistry> materialRegistry)
  {
  }

  void Generator::generateData()
  {
  }

  }  // namespace sg
} // namespace ospray
