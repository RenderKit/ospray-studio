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

  virtual void mainMethod(std::shared_ptr<StudioContext> ctx) = 0;
  PanelList panels;

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
