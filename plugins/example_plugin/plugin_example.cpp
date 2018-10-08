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

#include <memory>

#include "PanelExample.h"
#include "app/Plugin.h"

namespace ospray {
  namespace example_plugin {

    struct PluginExample : public Plugin
    {
      PluginExample() : Plugin("Example") {}

      PanelList createPanels(std::shared_ptr<sg::Frame>) override
      {
        PanelList panels;
        panels.emplace_back(new PanelExample());
        return panels;
      }
    };

    extern "C" Plugin *init_plugin_example()
    {
      std::cout << "loaded plugin 'example'!" << std::endl;
      return new PluginExample();
    }

  }  // namespace example_plugin
}  // namespace ospray
