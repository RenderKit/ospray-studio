// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include <memory>

#include "PanelExample.h"

#include "app/ospStudio.h"
#include "app/Plugin.h"

namespace ospray {
namespace example_plugin {

struct PluginExample : public Plugin
{
  PluginExample() : Plugin("Example") {}

  PanelList createPanels(std::shared_ptr<StudioContext> _context) override
  {
    PanelList panels;
    panels.emplace_back(new PanelExample(_context));
    return panels;
  }
};

extern "C" PLUGIN_INTERFACE Plugin *init_plugin_example()
{
  std::cout << "loaded plugin 'example'!" << std::endl;
  return new PluginExample();
}

} // namespace example_plugin
} // namespace ospray
