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

#include "../Panel.h"

#include "sg/SceneGraph.h"

#include <memory>
#include <utility>

namespace ospray {

  using LightPreset = std::pair<std::string, std::shared_ptr<sg::Node>>;

  struct PanelLights : public Panel
  {
    PanelLights(std::shared_ptr<sg::Frame> sg);

    void buildUI() override;

   private:

    void addLightModal(bool &show_add_light_modal);

    // Data //

    int numCreatedLights{1};
    std::shared_ptr<sg::Frame> scenegraph;
    std::shared_ptr<sg::Node> lights;
  };

}  // namespace ospray
