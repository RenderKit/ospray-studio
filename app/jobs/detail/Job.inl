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

// TEMPORARY //////////////////////////////////////////////////////////////////
// NOTE: TBB segfaults when using ospcommon::taksing::* to schedule work,
//       so use equivalent versions which always launch a std::thread for
//       now...
#define USE_OSPCOMMON_SCHEDULE 0

#if USE_OSPCOMMON_SCHEDULE
#include "ospcommon/tasking/async.h"
#else
#include "thread_task.h"
#endif

namespace ospray {
  namespace job_scheduler {

    inline Job::Job(Task task)
    {
      stashedTask   = task;
      jobFinished   = false;
      auto *thisJob = this;

#if USE_OSPCOMMON_SCHEDULE
      runningJob = ospcommon::tasking::async([=]() {
#else
      runningJob = detail::async([=]() {
#endif
        Nodes retval         = thisJob->stashedTask();
        thisJob->jobFinished = true;
        return retval;
      });
    }

    inline Job::~Job()
    {
      if (isValid())
        runningJob.wait();
    }

    inline bool Job::isFinished() const
    {
      return jobFinished;
    }

    inline bool Job::isValid() const
    {
      return runningJob.valid();
    }

    inline Nodes Job::get()
    {
      return runningJob.valid() ? runningJob.get() : Nodes{};
    }

  }  // namespace job_scheduler
}  // namespace ospray
