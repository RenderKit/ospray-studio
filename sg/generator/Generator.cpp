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

  void Generator::preCommit()
  {
    // Re-run generator on paramater changes in the UI
    if (child("parameters").isModified())
      generateData();
  }

  void Generator::postCommit()
  {
  }

  void Generator::generateData()
  {
  }

  }  // namespace sg
} // namespace ospray
