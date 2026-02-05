#include <memory>

#include "PanelStoryboard.h"

#include "app/ospStudio.h"
#include "app/Plugin.h"

namespace ospray {
namespace storyboard_plugin {

struct PluginStoryboard : public Plugin
{
  PluginStoryboard() : Plugin("Storyboard") {}

  void mainMethod(std::shared_ptr<StudioContext> ctx) override
  {
    if (ctx->mode == StudioMode::GUI) {
      auto &studioCommon = ctx->studioCommon;
      int ac = studioCommon.plugin_argc;
      const char **av = studioCommon.plugin_argv;

      std::string optPanelName = "Storyboard Panel";
      std::string configFilePath = "config/storyboard_settings.json";

      for (int i=0; i<ac; ++i) {
        std::string arg = av[i];
        if (arg == "--plugin:storyboard:name") {
          optPanelName = av[i + 1];
          ++i;
        }
        else if (arg == "--plugin:storyboard:config") {
          configFilePath = av[i + 1];
          ++i;
        }
      }

      panels.emplace_back(new PanelStoryboard(ctx, optPanelName, configFilePath));
    }
    else
      std::cout << "Plugin functionality unavailable in Batch mode .."
                << std::endl;
  }
};

extern "C" PLUGIN_INTERFACE Plugin *init_plugin_storyboard()
{
  std::cout << "loaded plugin 'storyboard'!" << std::endl;
  return new PluginStoryboard();
}

} // namespace storyboard_plugin
} // namespace ospray
