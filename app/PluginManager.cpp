// ======================================================================== //
// Copyright 2009-2018 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#include "PluginManager.h"

#include <iterator>

namespace ospray {

  void PluginManager::loadPlugin(const std::string &name)
  {
    std::cout << "...attempting to load module '" << name << "'\n";
    std::string libName = "ospray_studio_plugin_" + name;
    try {
      loadLibrary(libName, false);
    } catch (...) {
      std::cout
          << "...failed to load plugin '" << name << "'!"
          << " (plugin was not found). Please verify the name of the plugin"
          << " is correct and that it is on your LD_LIBRARY_PATH." << std::endl;
      return;
    }

    std::string initSymName = "init_plugin_" + name;
    void *initSym           = getSymbol(initSymName);
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

  PanelList PluginManager::getAllPanelsFromPlugins(
      std::shared_ptr<sg::Frame> scenegraph)
  {
    PanelList allPanels;

    for (auto &plugin : plugins) {
      auto panels = plugin.instance->createPanels(scenegraph);
      std::move(panels.begin(), panels.end(), std::back_inserter(allPanels));
    }

    return allPanels;
  }

}  // namespace ospray
