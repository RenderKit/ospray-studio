// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../Node.h"

namespace ospray {
  namespace sg {

    struct Search : public Visitor
    {
      Search(const std::string &s, const NodeType nt, std::vector<NodePtr> &v);

      bool operator()(Node &node, TraversalContext &ctx) override;

     private:
      NodeType type;
      std::string term;
      std::vector<NodePtr> &results;
    };

    // Inlined definitions ////////////////////////////////////////////////////

    inline Search::Search(
        const std::string &s, const NodeType nt, std::vector<NodePtr> &v)
        : type(nt), term(s), results(v)
    {
    }

    inline bool Search::operator()(Node &node, TraversalContext &)
    {
      if (type == NodeType::GENERIC || node.type() == type)
        if (node.name().find(term) != std::string::npos) {
          results.push_back(node.nodeAs<Node>());
          return false;
        }
      return true;
    }
  }  // namespace sg
}  // namespace ospray
