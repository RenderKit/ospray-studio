// Copyright 2018-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "Plugin.h"

using namespace ospray;
  // Helper types //
  struct LoadedPlugin
  {
    std::unique_ptr<Plugin> instance;
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

  // TODO: add functions to get a fresh set of panels, activate/deactive, etc.
  void main(
      std::shared_ptr<StudioContext> ctx, PanelList *allPanels = nullptr) const;
  void mainPlugin(std::shared_ptr<StudioContext> ctx,
    std::string &pluginName,
    PanelList *allPanels = nullptr) const;
  bool hasPlugin(const std::string &pluginName);

  LoadedPlugin *getPlugin(std::string &pluginName);

 private:
  // Helper functions //

  void addPlugin(std::unique_ptr<Plugin> plugin);

  // Data //

  std::vector<LoadedPlugin> plugins;
};
