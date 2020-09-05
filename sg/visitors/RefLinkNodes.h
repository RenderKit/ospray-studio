// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../Node.h"

using namespace rkcommon::math;

namespace ospray {
namespace sg {

struct RefLinkNodes : public Visitor
{
  RefLinkNodes() = default;
  ~RefLinkNodes() override = default;

  bool operator()(Node &node, TraversalContext &) override;

 private:
  std::map<std::string, Node *> referingNodes;
};

// Inlined definitions //////////////////////////////////////////////////////

inline bool RefLinkNodes::operator()(Node &node, TraversalContext &)
{
  auto nodeName = node.name();

  if (nodeName.substr(0, nodeName.find_first_of("_")) == "refLink") {
    auto &nParents = node.parents();
    // refLink Node should have only one parent
    if (nParents.size() == 1) {
      auto &refchild = node.child("id");
      auto refLinkIdString = refchild.valueAs<std::string>();
      auto &referingNode = nParents[0];
      referingNodes.insert(std::make_pair(refLinkIdString, nParents[0]));
    }
  }

  if (node.hasChild("asset_id")
      && node.child("asset_type").valueAs<std::string>() == "geometry") {
    auto &asset_id = node.child("asset_id").valueAs<std::string>();
    if (referingNodes.find(asset_id) != referingNodes.end()) {
      auto &newParent = referingNodes[asset_id];

      auto nodeParents = node.parents();
      for (auto &p : nodeParents) {
        newParent->add(node);
        p->remove(node);
      }
      referingNodes.erase(asset_id);
    }
  }
  return true;
}

} // namespace sg
} // namespace ospray