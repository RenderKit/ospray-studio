// Copyright 2018 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "PluginManager.h"

#include "sg/PluginCore.h"

#include <iterator>

void PluginManager::loadPlugin(const std::string &name)
{
  void *plugin = ospray::sg::loadPluginCore(name);
  if (plugin != nullptr) {
    auto pluginInstance =
       std::unique_ptr<Plugin>(static_cast<Plugin *>(plugin));
    addPlugin(std::move(pluginInstance));
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

bool PluginManager::hasPlugin(const std::string &pluginName)
{
  for (auto &p : plugins)
    if (p.instance->name() == pluginName)
      return true;
  return false;
}

LoadedPlugin* PluginManager::getPlugin(std::string &pluginName)
{
  for (auto &l : plugins)
    if (l.instance->name() == pluginName)
      return &l;

  return nullptr;
}

void PluginManager::main(
    std::shared_ptr<StudioContext> ctx, PanelList *allPanels) const
{
  if (!plugins.empty())
    for (auto &plugin : plugins) {
      plugin.instance->mainMethod(ctx);
      if (!plugin.instance->panels.empty() && allPanels) {
        auto &pluginPanels = plugin.instance->panels;
        std::move(pluginPanels.begin(),
            pluginPanels.end(),
            std::back_inserter(*allPanels));
      }
    }
}

void PluginManager::mainPlugin(std::shared_ptr<StudioContext> ctx,
    std::string &pluginName,
    PanelList *allPanels) const
{
  if (!plugins.empty())
    for (auto &plugin : plugins) {
      if (plugin.instance->name() == pluginName) {
        plugin.instance->mainMethod(ctx);
        if (!plugin.instance->panels.empty() && allPanels) {
          auto &pluginPanels = plugin.instance->panels;
          std::move(pluginPanels.begin(),
              pluginPanels.end(),
              std::back_inserter(*allPanels));
        }
      }
    }
}
