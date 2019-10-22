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

#pragma once

#include "Plugin.h"

#include <ospray/sg/SceneGraph.h>

namespace ospray {

  struct PluginManager
  {
    PluginManager()  = default;
    ~PluginManager() = default;

    void loadPlugin(const std::string &name);
    void removePlugin(const std::string &name);

    // TODO: add functions to get a fresh set of panels, activate/deactive, etc.
    PanelList getAllPanelsFromPlugins(
        std::shared_ptr<sg::Frame> scenegraph) const;

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

}  // namespace ospray
