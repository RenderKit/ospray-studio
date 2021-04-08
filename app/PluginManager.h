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
  PanelList getAllPanelsFromPlugins(
      std::shared_ptr<StudioContext> _context) const;
  // use following function to invoke main method of a plugin
  void callMainMethod(std::shared_ptr<StudioContext> _context) const;

 private:
  // Helper types //
  struct LoadedPlugin
  {
    std::unique_ptr<Plugin> instance;
    bool active{true};
  };

  // Helper functions //

  void addPlugin(std::unique_ptr<Plugin> plugin);

  // Data //

  std::vector<LoadedPlugin> plugins;
};

} // namespace ospray
