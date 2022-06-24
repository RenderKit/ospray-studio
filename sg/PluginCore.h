// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "Node.h"
#include <string>

// ospray sg

namespace ospray {
  namespace sg {

    OSPSG_INTERFACE void * loadPluginCore(const std::string &name);

  }  // namespace sg
} // namespace ospray
