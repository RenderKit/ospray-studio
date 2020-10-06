// Copyright 2018-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "PluginManager.h"

#include <iterator>

namespace ospray {

void PluginManager::loadPlugin(const std::string &name)
{
  std::cout << "...attempting to load plugin '" << name << "'\n";
  std::string libName = "ospray_studio_plugin_" + name;
  try {
    rkcommon::loadLibrary(libName, false);
  } catch (...) {
    std::cout << "...failed to load plugin '" << name << "'!"
              << " (plugin was not found). Please verify the name of the plugin"
              << " is correct and that it is on your LD_LIBRARY_PATH."
              << std::endl;
    return;
  }

  std::string initSymName = "init_plugin_" + name;
  void *initSym = rkcommon::getSymbol(initSymName);
  if (!initSym) {
    std::cout << "...failed to load plugin, could not find init function!\n";
    return;
  }

  Plugin *(*initMethod)() = (Plugin * (*)()) initSym;

  try {
    auto pluginInstance =
        std::unique_ptr<Plugin>(static_cast<Plugin *>(initMethod()));
    addPlugin(std::move(pluginInstance));
  } catch (...) {
    std::cout << "...failed to initialize plugin '" << name << "'!\n";
  }
}

void PluginManager::addPlugin(std::unique_ptr<Plugin> plugin)
{
  plugins.emplace_back(LoadedPlugin{std::move(plugin), true});
}

void PluginManager::removePlugin(const std::string &name)
{
  plugins.erase(std::remove_if(plugins.begin(), plugins.end(), [&](auto &p) {
    return p.instance->name() == name;
  }));
}

void PluginManager::removeAllPlugins()
{
  plugins.clear();
}

PanelList PluginManager::getAllPanelsFromPlugins(
    std::shared_ptr<StudioContext> _context) const
{
  PanelList allPanels;

  for (auto &plugin : plugins) {
    auto panels = plugin.instance->createPanels(_context);
    std::move(panels.begin(), panels.end(), std::back_inserter(allPanels));
  }

  return allPanels;
}

} // namespace ospray
