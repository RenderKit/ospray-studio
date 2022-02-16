// Copyright 2018-2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "Plugin.h"

  // Helper types //
  struct LoadedPlugin
  {
    std::unique_ptr<ospray::Plugin> instance;
    bool active{true};
  };

class PluginManager
{
  public:
  PluginManager() = default;
  ~PluginManager() = default;

  void loadPlugin(const std::string &name);
  void removePlugin(const std::string &name);
  void removeAllPlugins();

  // TODO: add functions to get a fresh set of panels, activate/deactivate, etc.
  void main(
      std::shared_ptr<StudioContext> ctx, ospray::PanelList *allPanels = nullptr) const;
  void mainPlugin(std::shared_ptr<StudioContext> ctx,
    std::string &pluginName,
    ospray::PanelList *allPanels = nullptr) const;
  bool hasPlugin(const std::string &pluginName);

  LoadedPlugin *getPlugin(std::string &pluginName);

 private:
  // Helper functions //

  void addPlugin(std::unique_ptr<ospray::Plugin> plugin);

  // Data //

  std::vector<LoadedPlugin> plugins;
};
