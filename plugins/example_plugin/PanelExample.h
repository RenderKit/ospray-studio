// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "app/widgets/Panel.h"
#include "app/ospStudio.h"

namespace ospray {
  namespace example_plugin {

    struct PanelExample : public Panel
    {
      PanelExample(std::shared_ptr<StudioContext> _context, std::string _panelName);

      void buildUI(void *ImGuiCtx) override;

    private:
      std::string panelName;
    };

  }  // namespace example_plugin
}  // namespace ospray
