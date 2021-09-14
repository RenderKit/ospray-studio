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
    if (ctx->mode == StudioMode::GUI) {
      auto &studioCommon = ctx->studioCommon;
      int ac = studioCommon.argc;
      const char **av = studioCommon.argv;

      std::string optPanelName = "Example Panel";

      for (int i=1; i<ac; ++i) {
        std::string arg = av[i];
        if (arg == "--plugin:example:name") {
          optPanelName = av[i + 1];
          ++i;
        }
      }

      panels.emplace_back(new PanelExample(ctx, optPanelName));
    }
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
