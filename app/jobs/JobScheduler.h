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

// std
#include <chrono>
#include <future>
#include <functional>
#include <vector>
#include <thread> // temporary - see below
// ospcommon
#include "ospcommon/tasking/async.h"
// ospray_sg
#include "sg/common/Node.h"

namespace ospray {
  namespace job_scheduler {

    // TEMPORARY //////////////////////////////////////////////////////////////

    // NOTE: TBB segfaults when using ospcommon::taksing::* to schedule work,
    //       so use equivalent versions which always launch a std::thread for
    //       now...

    namespace detail {

      template <typename TASK_T>
      inline void schedule(TASK_T fcn)
      {
        std::thread thread(fcn);
        thread.detach();
      }

      template<typename TASK_T>
      using operator_return_t = typename std::result_of<TASK_T()>::type;

      template<typename TASK_T>
      inline auto async(TASK_T&& fcn) -> std::future<operator_return_t<TASK_T>>
      {
        using package_t = std::packaged_task<operator_return_t<TASK_T>()>;

        auto task   = new package_t(std::forward<TASK_T>(fcn));
        auto future = task->get_future();

        schedule([=](){ (*task)(); delete task; });

        return future;
      }

    } // namespace detail

    ///////////////////////////////////////////////////////////////////////////

    // Helper types //

    using Nodes = std::vector<std::shared_ptr<sg::Node>>;

    struct Job
    {
      template <typename TASK_T>
      Job(TASK_T &&task)
      {
        stashedTask = task;
        jobFinished = false;
        auto *thisJob = this;

#if 0
        runningJob = ospcommon::tasking::async([=](){
#else
        runningJob = detail::async([=](){
#endif
          Nodes retval = thisJob->stashedTask();
          thisJob->jobFinished = true;
          return retval;
        });
      }

      ~Job() { if (isValid()) runningJob.wait(); }

      bool isFinished() const { return jobFinished; }
      bool isValid() const { return runningJob.valid(); }

      Nodes get() { return runningJob.valid() ? runningJob.get() : Nodes{}; }

    private:

      std::function<Nodes()> stashedTask;
      std::future<Nodes> runningJob;
      std::atomic<bool> jobFinished;
    };

    // Main Job Scheduler API /////////////////////////////////////////////////

    template <typename JOB_T>
    inline std::unique_ptr<Job> schedule_job(JOB_T &&job)
    {
      return ospcommon::make_unique<Job>(std::forward<JOB_T>(job));
    }

  } // namespace job_scheduler
} // namespace ospray
