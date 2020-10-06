// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../Node.h"

namespace ospray {
  namespace sg {

  struct CommitVisitor : public Visitor
  {
    CommitVisitor()           = default;
    CommitVisitor(bool _force) : force(_force) {}
    ~CommitVisitor() override = default;

    bool operator()(Node &node, TraversalContext &) override;
    void postChildren(Node &node, TraversalContext &) override;
    protected:
    bool force{false};
  };

  // Inlined definitions //////////////////////////////////////////////////////

  inline bool CommitVisitor::operator()(Node &node, TraversalContext &)
  {
    if (force || node.subtreeModifiedButNotCommitted()) {
      node.preCommit();
      return true;
    } else {
      return false;
    }
  }

  inline void CommitVisitor::postChildren(Node &node, TraversalContext &)
  {
    if (force || node.subtreeModifiedButNotCommitted()) {
      node.postCommit();
      node.properties.lastCommitted.renew();
    }
  }

  }  // namespace sg
} // namespace ospray
