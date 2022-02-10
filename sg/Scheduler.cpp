// Copyright 2018-2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Scheduler.h"

namespace ospray {
namespace sg {
namespace scheduler {


SchedulerPtr Scheduler::create() {
  SchedulerPtr scheduler = std::make_shared<Scheduler>(key{});

  scheduler->osprayInstance = std::make_shared<Instance>(scheduler, OSPRAY, "ospray");
  scheduler->studioInstance = std::make_shared<Instance>(scheduler, STUDIO, "studio");
  scheduler->backgroundInstance = std::make_shared<Instance>(scheduler, BACKGROUND, "background");

  scheduler->nameToId.insert({
    {"ospray", OSPRAY},
    {"studio", STUDIO},
    {"background", BACKGROUND},
  });

  scheduler->idToInstance.insert({
    {OSPRAY, scheduler->osprayInstance},
    {STUDIO, scheduler->studioInstance},
    {BACKGROUND, scheduler->backgroundInstance},
  });

  return scheduler;
}

InstancePtr Scheduler::lookup(size_t id) {
  std::lock_guard<std::mutex> lock(mutex);
  return lookupById(id);
}

InstancePtr Scheduler::lookup(const std::string &name) {
  std::lock_guard<std::mutex> lock(mutex);
  InstancePtr instance = lookupByName(name);
  if (instance == nullptr) {
    instance = addByName(name);
  }
  return instance;
}

InstancePtr Scheduler::lookupById(size_t id) const {
  auto it = idToInstance.find(id);
  if (it == idToInstance.end()) {
    return nullptr;
  }

  return it->second;
}

InstancePtr Scheduler::lookupByName(const std::string &name) const {
  auto it = nameToId.find(name);
  if (it == nameToId.end()) {
    return nullptr;
  }

  size_t id = it->second;
  return lookupById(id);
}

InstancePtr Scheduler::addByName(const std::string &name) {
  std::fprintf(stderr, "Scheduler: create new scheduler instance with name: %s\n",
               name.c_str());

  nameToId.emplace(name, nextId);
  InstancePtr instance = std::make_shared<Instance>(shared_from_this(), nextId, name);
  idToInstance.emplace(nextId, instance);
  ++nextId;
  return instance;
}


void Instance::push(const Function &fcn) {
  push("<unnamed task>", fcn);
}

void Instance::push(const std::string &name, const Function &fcn) {
  std::fprintf(stderr, "Scheduler(%s): schedule new task with name: %s\n",
               this->name.c_str(), name.c_str());

  std::lock_guard<std::mutex> lock(mutex);
  TaskPtr task = std::make_shared<Task>(shared_from_this(), name, std::make_shared<Function>(fcn));
  tasks.emplace(task);
}

TaskPtr Instance::pop() {
  TaskPtr task{nullptr};

  {
    std::lock_guard<std::mutex> lock(mutex);
    if (tasks.empty()) {
      return nullptr;
    }
    task = tasks.front();
    tasks.pop();
  }

  std::fprintf(stderr, "Scheduler(%s): pulled task with name: %s\n",
               this->name.c_str(), task->name.c_str());

  return task;
}

size_t Instance::executeAllTasksSync() {
  TaskPtr task = pop();
  return executeAllTasksSync(task);
}

size_t Instance::executeAllTasksSync(const TaskPtr &first) {
  size_t numTasksExecuted = 0;

  for (TaskPtr task = first; task; task = pop()) {
    ++numTasksExecuted;
    (*task)();
  }

  return numTasksExecuted;
}

size_t Instance::executeAllTasksAsync() {
  TaskPtr task = pop();
  return executeAllTasksAsync(task);
}

size_t Instance::executeAllTasksAsync(const TaskPtr &first) {
  size_t numTasksExecuted = 0;

  // ensure this object survives through all lambdas
  auto self = shared_from_this();

  for (TaskPtr task = first; task; task = pop()) {
    ++numTasksExecuted;

    // the callback takes the task (task is an std::shared_ptr) by value,
    // increasing the refcount, and ensuring the object stays alive throughout
    // the function call
    auto callback = [task, self]() {
      try {
        (*task)();

      } catch (...) {
        std::lock_guard<std::mutex> lock(self->mutex);
        self->running.erase(task);
        throw;
      }
      std::lock_guard<std::mutex> lock(self->mutex);
      self->running.erase(task);
    };

    {
      std::lock_guard<std::mutex> lock(mutex);
      running.emplace(task);
    }

    std::thread t(callback);
    t.detach();
  }

  return numTasksExecuted;
}

size_t Instance::wait() {
  using namespace std::chrono_literals;
  size_t numTasksWaited = 0;

  {
    std::lock_guard<std::mutex> lock(mutex);
    numTasksWaited = running.size();
  }

  if (numTasksWaited == 0) {
    return numTasksWaited;
  }

  for (;;) {
    {
      std::lock_guard<std::mutex> lock(mutex);
      if (running.empty()) {
        return numTasksWaited;
      }
    }

    std::this_thread::sleep_for(100ms);
  }
}


void Task::operator()() const {
  std::fprintf(stderr, "Scheduler(%s): start task with name: %s\n",
               instance->name.c_str(), name.c_str());

  try {
    fcn->operator()(instance->scheduler);
  } catch (...) {
    std::fprintf(stderr, "Scheduler(%s): got exception in task with name: %s\n",
                 instance->name.c_str(), name.c_str());

    throw;
  }

  std::fprintf(stderr, "Scheduler(%s): finished task with name: %s\n",
               instance->name.c_str(), name.c_str());
}


} // namespace scheduler
} // namespace sg
} // namespace ospray
