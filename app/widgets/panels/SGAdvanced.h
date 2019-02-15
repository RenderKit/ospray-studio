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

namespace ospray {

  struct PanelSGAdvanced : public Panel
  {
    PanelSGAdvanced(std::shared_ptr<sg::Frame> sg);

    void buildUI() override;

   private:
    // Data //

    std::shared_ptr<sg::Frame> scenegraph;
  };

}  // namespace ospray
