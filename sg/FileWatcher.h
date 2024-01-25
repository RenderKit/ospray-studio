// Copyright 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "rkcommon/os/FileName.h"
#include "sg/Node.h"

namespace ospray {
namespace sg {

// Global watch list checker
void checkFileWatcherModifications();

__inline void addFileWatcher(
    Node &parent, std::string watchName, bool autoUpdate)
{
  auto watchNodeName = watchName + " autoUpdate";
  if (!parent.hasChild(watchNodeName)) {
    parent.createChild(watchNodeName, "fileWatcher", autoUpdate);
    parent.child(watchNodeName).setSGOnly();
  }
}

struct OSPSG_INTERFACE FileWatcher : public BoolNode
{
  FileWatcher();
  ~FileWatcher();

  void preCommit() override;
  void postCommit() override;

  bool fileModified{false};
  time_t lastModTime{0};
};

} // namespace sg
} // namespace ospray

