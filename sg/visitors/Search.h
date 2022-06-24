// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../Node.h"

namespace ospray {
  namespace sg {

    // This is a vector of weak_ptr so as not to increase reference count on
    // result nodes
    using SearchResults = std::vector<std::weak_ptr<Node>>;

    struct Search : public Visitor
    {
      Search(const std::string &s, const NodeType nt, SearchResults &v);

      bool operator()(Node &node, TraversalContext &ctx) override;

     private:
      NodeType type;
      std::string term;
      SearchResults &results;
    };

    // Inlined definitions ////////////////////////////////////////////////////

    inline Search::Search(
        const std::string &s, const NodeType nt, SearchResults &v)
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
