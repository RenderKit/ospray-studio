// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../Node.h"

namespace ospray {
  namespace sg {

    struct Search : public Visitor
    {
      Search(const std::string &s, const NodeType nt, std::vector<Node *> &v);

      bool operator()(Node &node, TraversalContext &ctx) override;

     private:
      NodeType type;
      std::string term;
      std::vector<Node *> &results;
    };

    // Inlined definitions ////////////////////////////////////////////////////

    Search::Search(const std::string &s,
                   const NodeType nt,
                   std::vector<Node *> &v)
        : term(s), type(nt), results(v)
    {
    }

    inline bool Search::operator()(Node &node, TraversalContext &ctx)
    {
      if (type == NodeType::GENERIC || node.type() == type)
        if (node.name().find(term) != std::string::npos)
          results.push_back(&node);
      return true;
    }
  }  // namespace sg
}  // namespace ospray
