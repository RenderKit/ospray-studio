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

#include "ospray/sg/SceneGraph.h"

// tf editor widget
#include "../transfer_function/TransferFunctionWidget.h"
using tfn::tfn_widget::TransferFunctionWidget;

namespace ospray {

  struct PanelTFEditor : public Panel
  {
    PanelTFEditor(std::shared_ptr<sg::TransferFunction> tfn,
                   const std::vector<std::string> &tfnsToLoad);

    void buildUI() override;

   private:
    // Data //

    std::shared_ptr<TransferFunctionWidget> widget;
  };

}  // namespace ospray
