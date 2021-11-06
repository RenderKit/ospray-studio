// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../Node.h"

namespace ospray {
namespace sg {

struct CollectTransferFunctions : public Visitor
{
  CollectTransferFunctions() = default;

  bool operator()(Node &node, TraversalContext &ctx) override;

  std::map<std::string, NodePtr> transferFunctions;
};

// Inlined definitions //////////////////////////////////////////////////////

inline bool CollectTransferFunctions::operator()(Node &node, TraversalContext &ctx)
{
  switch (node.type()) {
  case NodeType::TRANSFER_FUNCTION: {
    transferFunctions[ctx.name] = node.nodeAs<Node>();
    break;
  }
  }
  return true;
}

}  // namespace sg
} // namespace ospray