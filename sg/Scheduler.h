// Copyright 2018-2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <array>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>

namespace ospray {
namespace sg {

#define CONTEXTLIST                                                            \
  X(foreground)                                                                \
  X(background)

#define X(context) context,
enum class SchedulerContextType {
  CONTEXTLIST
};
#undef X

#define X(context) {SchedulerContextType::context, #context},
static std::map<SchedulerContextType, std::string> SchedulerContextTypeToString = {
  CONTEXTLIST
};
#undef X

#define X(context) {#context, SchedulerContextType::context},
static std::map<std::string, SchedulerContextType> SchedulerContextTypeFromString = {
  CONTEXTLIST
};
#undef X

#define X(context) struct context##_t : public std::integral_constant<SchedulerContextType, SchedulerContextType::context> {};
CONTEXTLIST
#undef X

#define X(context) static const context##_t context = context##_t();
CONTEXTLIST
#undef X

#undef CONTEXTLIST


class Scheduler;
using SchedulerPtr = std::shared_ptr<Scheduler>;

class Scheduler : public std::enable_shared_from_this<Scheduler> {
public:

  class Task;
  using TaskPtr = std::shared_ptr<Task>;

  using Function = std::function<void(SchedulerPtr)>;
  using FunctionPtr = std::shared_ptr<Function>;

  class Task : public std::enable_shared_from_this<Task> {
    friend Scheduler;

  public:
    Task() = delete;
    ~Task() = default;
    Task(const Task &) = delete;
    Task &operator=(const Task &) = delete;

    void operator()() {
      if (context == SchedulerContextType::background) {
        // this level of indirection (the "self" variable) is necessary or else
        // this Task may be destroyed before or during the execute call and lead
        // to a segfault
        auto self = shared_from_this();
        std::thread t([self]() { self->execute(); });
        t.detach();

      } else if (context == SchedulerContextType::foreground) {
        execute();
      }
    }

  private:
    Task(SchedulerPtr _scheduler, SchedulerContextType _context, std::string _name, FunctionPtr _function)
      : scheduler(_scheduler)
      , context(_context)
      , name(_name)
      , function(_function)
    {};

    void execute() {
      logStart();

      try {
        function->operator()(scheduler);
      } catch (std::exception &e) {
        std::cerr << "Scheduler::Task encountered exception:" << std::endl;
        std::cerr << "    " << e.what() << std::endl;
      } catch (...) {
        std::cerr << "Scheduler::Task encountered unknown exception" << std::endl;
      }

      logFinish();
    }

    void logCreate() {
      std::fprintf(stderr, "Scheduler:%s: task %s created\n", SchedulerContextTypeToString[context].c_str(), name.c_str());
    }

    void logStart() {
      std::fprintf(stderr, "Scheduler:%s: task %s started\n", SchedulerContextTypeToString[context].c_str(), name.c_str());
    }

    void logFinish() {
      std::fprintf(stderr, "Scheduler:%s: task %s finished\n", SchedulerContextTypeToString[context].c_str(), name.c_str());
    }

    SchedulerPtr scheduler;
    SchedulerContextType context;
    FunctionPtr function;
    std::string name;
  };

  Scheduler() = default;
  ~Scheduler() = default;

  void push(foreground_t tag, std::string name, Function fcn) {
    auto task = make_shared_task(tag.value, name, std::make_shared<Function>(fcn));
    push(foregroundMutex, foregroundTasks, task);
  }

  void push(foreground_t tag, Function fcn) {
    push(tag, "<unnamed foreground task>", fcn);
  }

  void push(background_t tag, std::string name, Function fcn) {
    auto task = make_shared_task(tag.value, name, std::make_shared<Function>(fcn));
    push(backgroundMutex, backgroundTasks, task);
  }

  void push(background_t tag, Function fcn) {
    push(tag, "<unnamed background task>", fcn);
  }

  TaskPtr pop(foreground_t) {
    return pop(foregroundMutex, foregroundTasks);
  }

  TaskPtr pop(background_t) {
    return pop(backgroundMutex, backgroundTasks);
  }

private:
  TaskPtr make_shared_task(SchedulerContextType context, std::string name, FunctionPtr fcn) {
    return TaskPtr(new Task(shared_from_this(), context, name, fcn));
  }

  void push(std::mutex &mutex, std::queue<TaskPtr> &tasks, TaskPtr task) {
    {
      std::lock_guard<std::mutex> lock(mutex);
      tasks.emplace(task);
    }
    task->logCreate();
  }

  TaskPtr pop(std::mutex &mutex, std::queue<TaskPtr> &tasks) {
    std::lock_guard<std::mutex> lock(mutex);
    if (tasks.empty()) {
      return nullptr;
    }

    TaskPtr task = tasks.front();
    tasks.pop();

    return task;
  }

  std::mutex foregroundMutex{};
  std::queue<TaskPtr> foregroundTasks{};

  std::mutex backgroundMutex{};
  std::queue<TaskPtr> backgroundTasks{};
};

} // namespace sg
} // namespace ospray
