// Copyright 2018-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "Plugin.h"

#include "ospStudio.h"

namespace ospray {

struct PluginManager
{
  PluginManager() = default;
  ~PluginManager() = default;

  void loadPlugin(const std::string &name);
  void removePlugin(const std::string &name);
  void removeAllPlugins();

  // TODO: add functions to get a fresh set of panels, activate/deactive, etc.
  void main(std::shared_ptr<StudioContext> ctx, PanelList *allPanels = nullptr) const;

  bool hasPlugin(const std::string &pluginName)
  {
    for (auto &p : plugins)
      if (p.instance->name() == pluginName)
        return true;
    return false;
  }

  // Helper types //
  struct LoadedPlugin
  {
    std::unique_ptr<Plugin> instance;
    bool active{true};
  };

  LoadedPlugin *getPlugin(std::string &pluginName)
  {
    for (auto &l : plugins)
      if (l.instance->name() == pluginName)
        return &l;

    return nullptr;
  }

 private:
  // Helper functions //

  void addPlugin(std::unique_ptr<Plugin> plugin);

  // Data //

  std::vector<LoadedPlugin> plugins;
};

} // namespace ospray
