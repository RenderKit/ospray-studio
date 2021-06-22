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

  void mainMethod(std::shared_ptr<StudioContext> ctx) override
  {
    if (ctx->mode == StudioMode::GUI)
      panels.emplace_back(new PanelExample(ctx));
    else
      std::cout << "Plugin functionality unavailable in Batch mode .."
                << std::endl;
  }
};

extern "C" PLUGIN_INTERFACE Plugin *init_plugin_example()
{
  std::cout << "loaded plugin 'example'!" << std::endl;
  return new PluginExample();
}

} // namespace example_plugin
} // namespace ospray
