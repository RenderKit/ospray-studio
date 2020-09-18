// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../Node.h"

namespace ospray {
namespace sg {

struct GenerateAnimationTimeline : public Visitor
{
  GenerateAnimationTimeline() = default;
  ~GenerateAnimationTimeline() override = default;

  bool operator()(Node &node, TraversalContext &) override;
  NodePtr currentParent;
  NodePtr currentWorld;

  std::map<std::string, NodePtr> nodeMap;
};

// Inlined definitions //////////////////////////////////////////////////////

inline bool GenerateAnimationTimeline::operator()(
    Node &node, TraversalContext &ctx)
{
  bool traverseChildren = true;

  switch (node.type()) {
    case NodeType::WORLD:
    currentWorld = ctx.animationWorld;
    break;

  case NodeType::GEOMETRY: {
  
    currentParent->add(node.shared_from_this());
    traverseChildren = false;
  } break;

  case NodeType::TRANSFORM: {

    auto &np = node.parents().front();
    auto xfmNode = node.shared_from_this();
    if (np->type() == NodeType::IMPORTER) {
      currentWorld->add(xfmNode);             
    } else if (currentParent->name() == np->name()){
      currentParent->add(xfmNode);      
    } else if (nodeMap.find(np->name()) != nodeMap.end()) {
      auto parentIter = nodeMap.find(np->name());
      parentIter->second->add(xfmNode);
    }
    currentParent = xfmNode;
    if(node.children().size() > 1)
      nodeMap.insert(std::make_pair(node.name(), node.shared_from_this()));
      }
  break;

  case NodeType::ANIMATION: {
    traverseChildren = false;
  } break;

  default:
    break;
  }
  return traverseChildren;
}
} // namespace sg
} // namespace ospray
