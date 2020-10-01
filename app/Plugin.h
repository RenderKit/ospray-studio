// Copyright 2018-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <memory>
#include <vector>

#include "widgets/Panel.h"

#include "sg/Frame.h"

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

  // Create an instance of each panel, the parameter passed in the is root
  // node in the scene graph
  virtual PanelList createPanels(std::shared_ptr<sg::Frame> frame) = 0;

  std::string name() const;

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

inline Plugin::Plugin(const std::string &_name) : pluginName(_name) {}

} // namespace ospray
