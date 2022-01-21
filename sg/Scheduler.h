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

struct scheduler_tag_t {};

struct on_thread_t : std::integral_constant<size_t, 0>, scheduler_tag_t {};
static const on_thread_t on_thread = on_thread_t();

struct off_thread_t : std::integral_constant<size_t, 1>, scheduler_tag_t {};
static const off_thread_t off_thread = off_thread_t();

class Scheduler;
using SchedulerPtr = std::shared_ptr<Scheduler>;

class Scheduler : public std::enable_shared_from_this<Scheduler> {
public:
  Scheduler() = default;
  ~Scheduler() = default;

  using Task = std::function<void(SchedulerPtr)>;
  using TaskPtr = std::shared_ptr<Task>;

  void schedule(on_thread_t tag, Task fcn) {
    schedule((size_t)tag, std::make_shared<Task>(fcn));
  }

  void schedule(off_thread_t tag, Task fcn) {
    schedule((size_t)tag, std::make_shared<Task>(fcn));
  }

  TaskPtr steal(on_thread_t tag) {
    return steal((size_t)tag);
  }

  TaskPtr steal(off_thread_t tag) {
    return steal((size_t)tag);
  }

  bool execute(on_thread_t tag) {
    return execute(tag, steal(tag));
  }

  bool execute(off_thread_t tag) {
    return execute(tag, steal(tag));
  }

  bool execute(on_thread_t tag, TaskPtr task) {
    if (!task) {
      return false;
    }

    Scheduler::run(task, shared_from_this());

    return true;
  }

  bool execute(off_thread_t tag, TaskPtr task) {
    if (!task) {
      return false;
    }

    std::thread thread(Scheduler::run, task, shared_from_this());
    thread.detach();

    return true;
  }

private:
  void schedule(size_t index, TaskPtr task) {
    std::lock_guard<std::mutex> lock(mutexes[index]);
    tasks[index].push(task);
  }

  TaskPtr steal(size_t index) {
    std::lock_guard<std::mutex> lock(mutexes[index]);
    if (tasks[index].empty()) {
      return nullptr;
    }

    auto task = tasks[index].front();
    tasks[index].pop();

    return task;
  }

  static void run(TaskPtr task, SchedulerPtr scheduler) {
    try {
      task->operator()(scheduler);
    } catch (std::exception &e) {
      std::cerr << "Scheduler task encountered exception:" << std::endl;
      std::cerr << "    " << e.what() << std::endl;
    } catch (...) {
      std::cerr << "Scheduler task encountered unknown exception" << std::endl;
    }
  }

  std::array<std::mutex, 2> mutexes;
  std::array<std::queue<TaskPtr>, 2> tasks;
};

} // namespace sg
} // namespace ospray
