// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "PluginCore.h"

#include <iostream>

#include <rkcommon/common.h>

namespace ospray {
namespace sg {

void * loadPluginCore(const std::string &name) {
  void *ret = nullptr;
  std::cout << "...attempting to load plugin '" << name << "'\n";
  std::string libName = "ospray_studio_plugin_" + name;
  try {
    rkcommon::loadLibrary(
        libName, reinterpret_cast<const void *>(&loadPluginCore));
  } catch (std::runtime_error &e) {
    std::cout << "...failed to load plugin '" << name << "'!"
              << " (plugin was not found). Please verify the name of the plugin"
              << " is correct and that it is on your LD_LIBRARY_PATH."
              << e.what() << std::endl;
    return ret;
  }
  std::string initSymName = "init_plugin_" + name;
  void *initSym = rkcommon::getSymbol(initSymName);
  if (!initSym) {
    std::cout << "...failed to load plugin, could not find init function!\n";
    return ret;
  }
  void *(*initMethod)() = (void * (*)()) initSym;
  try {
     ret = initMethod();
  } catch (const std::exception &e) {
    std::cout << "...failed to initialize plugin '" << name << "'!\n";
    std::cout << "  " << e.what() << std::endl;
  } catch (...) {
    std::cout << "...failed to initialize plugin '" << name << "'!\n";
  }

  return ret;
}

} // namespace sg
} // namespace ospray
