#include <memory>

#include "PanelGesture.h"

#include "app/ospStudio.h"
#include "app/Plugin.h"

namespace ospray {
namespace gesture_plugin {

struct PluginGesture : public Plugin
{
  PluginGesture() : Plugin("Gesture") {}

  void mainMethod(std::shared_ptr<StudioContext> ctx) override
  {
    if (ctx->mode == StudioMode::GUI) {
      auto &studioCommon = ctx->studioCommon;
      int ac = studioCommon.plugin_argc;
      const char **av = studioCommon.plugin_argv;

      std::string optPanelName = "Gesture Panel";
      std::string configFilePath = "config/tracking_settings.json";

      for (int i=0; i<ac; ++i) {
        std::string arg = av[i];
        if (arg == "--plugin:gesture:name") {
          optPanelName = av[i + 1];
          ++i;
        }
        else if (arg == "--plugin:gesture:config") {
          configFilePath = av[i + 1];
          ++i;
        }
      }

      panels.emplace_back(new PanelGesture(ctx, optPanelName, configFilePath));
    }
    else
      std::cout << "Plugin functionality unavailable in Batch mode .."
                << std::endl;
  }
};

extern "C" PLUGIN_INTERFACE Plugin *init_plugin_gesture()
{
  std::cout << "loaded plugin 'gesture'!" << std::endl;
  return new PluginGesture();
}

} // namespace gesture_plugin
} // namespace ospray
