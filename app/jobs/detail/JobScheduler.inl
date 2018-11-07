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

#include "../../widgets/MainWindow.h"

namespace ospray {
  namespace job_scheduler {

    inline void scheduleJob(Task task)
    {
      auto *mw = MainWindow::g_instance;
      if (mw == nullptr) {
        throw std::runtime_error(
            "FATAL: make MainWindow instance before calling scheduleJob()!");
      }

      mw->addJob(task);
    }

    inline void scheduleNodeValueChange(sg::Node &node, utility::Any value)
    {
      AsyncRenderEngine::g_instance->scheduleNodeValueChange(node, value);
    }

    inline void scheduleNodeOp(std::function<void()> op)
    {
      AsyncRenderEngine::g_instance->scheduleNodeOp(std::move(op));
    }

  } // namespace job_scheduler
} // namespace ospray
