// Copyright 2018-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <memory>
#include <vector>

#include "widgets/Panel.h"

#include "ospStudio.h"

#ifndef PLUGIN_INTERFACE
  #ifdef _WIN32
    #define PLUGIN_INTERFACE __declspec(dllexport)
  #else
    #define PLUGIN_INTERFACE
  #endif
#endif

namespace ospray {

struct Plugin
{
  Plugin() = default;
  virtual ~Plugin() = default;

  // Create an instance of each panel, the parameter passed in the is the
  // current application context
  virtual PanelList createPanels(std::shared_ptr<StudioContext> _context) = 0;
  virtual void mainMethod(std::shared_ptr<StudioContext> _context);

  std::string name() const;
  bool hasMainMethod{false};

 protected:
  Plugin(const std::string &pluginName);

 private:
  std::string pluginName;
};

// Inlined members //////////////////////////////////////////////////////////

inline std::string Plugin::name() const
{
  return pluginName;
}

inline void Plugin::mainMethod(std::shared_ptr<StudioContext> _context)
{}

inline Plugin::Plugin(const std::string &_name) : pluginName(_name) {}

} // namespace ospray
