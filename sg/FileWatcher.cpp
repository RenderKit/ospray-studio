// Copyright 2023 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "FileWatcher.h"

#include <sys/stat.h>
#include <sys/types.h>
#ifndef WIN32
#include <unistd.h>
#endif

#ifdef WIN32
#define stat _stat
#endif

namespace ospray {
namespace sg {

// Watcher Callback

static std::vector<FileWatcher *> FileWatcherList;

void checkFileWatcherModifications()
{
  if (FileWatcherList.empty())
    return;

  for (auto node : FileWatcherList) {
    if (!node)
      continue;

    // Only watch if watcher is active
    auto watcher = node->nodeAs<FileWatcher>();
    if (!watcher->valueAs<bool>())
      continue;

    // The watcher must have a parent node.
    if (!watcher->hasParents())
      continue;

    // Grab the filename from the watcher's parent
    auto &parentNode = watcher->parents().front();
    if (parentNode->subType() != "filename")
      continue;
    std::string filename =
        rkcommon::FileName(parentNode->valueAs<std::string>()).canonical();

    // Check the file modification time
    struct stat result;
    if (stat(filename.c_str(), &result) == 0) {
      auto modTime = result.st_mtime;
      if (watcher->lastModTime != modTime) {
        watcher->lastModTime = modTime;
        watcher->fileModified = true;
        // Mark node as modified to notify SG hierarchy
        watcher->markAsModified();
        //std::cout << watcher->name() << " " << filename << std::endl;
      }
    }
  }
}

// FileWatcher class definitions

FileWatcher::FileWatcher()
{
  //std::cout << "Create file watcher" << std::endl;
  FileWatcherList.push_back(this);
}

FileWatcher::~FileWatcher()
{
  //std::cout << "Destroy file watcher" << std::endl;
  FileWatcherList.erase(
      std::remove(std::begin(FileWatcherList), std::end(FileWatcherList), this),
      std::end(FileWatcherList));
  FileWatcherList.shrink_to_fit();
}

void FileWatcher::preCommit()
{
  //std::cout << "FileWatcher::preCommit() " << name() << ": " << valueAs<bool>()
  //          << std::endl;

  // Finish node preCommit
  Node::preCommit();
}

void FileWatcher::postCommit()
{
  //std::cout << "FileWatcher::postCommit() " << name() << std::endl;

  // Finish node postCommit
  Node::postCommit();
}

OSP_REGISTER_SG_NODE_NAME(FileWatcher, fileWatcher);

} // namespace sg
} // namespace ospray

