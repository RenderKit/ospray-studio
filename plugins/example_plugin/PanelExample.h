// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "app/widgets/Panel.h"

namespace ospray {
  namespace example_plugin {

    struct PanelExample : public Panel
    {
      PanelExample();

      void buildUI() override;
    };

  }  // namespace example_plugin
}  // namespace ospray
