// Copyright 2018-2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <array>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <functional>
#include <map>

namespace ospray {
namespace sg {
namespace scheduler {


class Scheduler;
using SchedulerPtr = std::shared_ptr<Scheduler>;

class Instance;
using InstancePtr = std::shared_ptr<Instance>;

class Task;
using TaskPtr = std::shared_ptr<Task>;

using Function = std::function<void(SchedulerPtr)>;
using FunctionPtr = std::shared_ptr<Function>;


class Scheduler : public std::enable_shared_from_this<Scheduler> {
protected:
  // passkey idiom https://chromium.googlesource.com/chromium/src/+/HEAD/docs/patterns/passkey.md
  //
  // This allows the constructor to be public while also only allowing this
  // class to call the constructor. This is needed to allow std::make_shared
  // calls within this class to access the constructor without allowing access
  // to everyone.
  struct key { explicit key() = default; };

public:
  enum {
    OSPRAY = 1001,
    STUDIO = 1002,
    BACKGROUND = 1003,
  };

  Scheduler(key) {}
  ~Scheduler() = default;
  Scheduler(const Scheduler &) = delete;
  Scheduler &operator=(const Scheduler &) = delete;

  static SchedulerPtr create();
  InstancePtr lookup(size_t id);
  InstancePtr lookup(const std::string &name);

  InstancePtr ospray() const {
    return osprayInstance;
  }

  InstancePtr studio() const {
    return studioInstance;
  }

  InstancePtr background() const {
    return backgroundInstance;
  }

private:
  InstancePtr lookupById(size_t id) const;
  InstancePtr lookupByName(const std::string &name) const;
  InstancePtr addByName(const std::string &name);

  std::mutex mutex{};
  size_t nextId{1};
  std::map<std::string, size_t> nameToId{};
  std::map<size_t, InstancePtr> idToInstance{};

  InstancePtr osprayInstance{};
  InstancePtr studioInstance{};
  InstancePtr backgroundInstance{};
};


class Instance : public std::enable_shared_from_this<Instance> {
public:
  Instance(SchedulerPtr _scheduler, size_t _id, const std::string &_name)
    : scheduler(_scheduler)
    , id(_id)
    , name(_name)
  {}

  ~Instance() = default;
  Instance(const Instance &) = delete;
  Instance &operator=(const Instance &) = delete;

  void push(const Function &fcn);
  void push(const std::string &name, const Function &fcn);
  TaskPtr pop();

  SchedulerPtr scheduler;
  size_t id;
  std::string name;

private:
  std::mutex mutex{};
  std::queue<TaskPtr> tasks{};
};


class Task : public std::enable_shared_from_this<Task> {
public:
  Task(InstancePtr _instance, const std::string &_name, FunctionPtr _fcn)
    : instance(_instance)
    , name(_name)
    , fcn(_fcn)
  {}

  ~Task() = default;
  Task(const Task &) = delete;
  Task &operator=(const Task &) = delete;

  void operator()() const;

  InstancePtr instance;
  std::string name;

private:
  FunctionPtr fcn;
};


} // namespace scheduler


using Scheduler = scheduler::Scheduler;
using SchedulerPtr = scheduler::SchedulerPtr;


} // namespace sg
} // namespace ospray
