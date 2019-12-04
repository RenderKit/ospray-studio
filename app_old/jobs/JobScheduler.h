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

#include "Task.h"

namespace ospray {
  namespace job_scheduler {

    // Main Job Scheduler API /////////////////////////////////////////////////

    // Schedule a generic, Node generating task on a background thread
    void scheduleJob(Task task);

    // Schedule scene graph node changes on the background rendering thread
    void scheduleNodeValueChange(sg::Node &node, utility::Any value);
    void scheduleNodeOp(std::function<void()> op);

  } // namespace job_scheduler
} // namespace ospray

#include "detail/JobScheduler.inl"
