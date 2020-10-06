// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../Node.h"
#include "../visitors/PrintNodes.h"

namespace ospray {
  namespace sg {

  struct GenerateAnimationSequence : public Visitor
  {
    GenerateAnimationSequence()           = default;
    ~GenerateAnimationSequence() override = default;

    bool operator()(Node &node, TraversalContext &ctx) override;
    void postChildren(Node &node, TraversalContext &ctx) override;

    private:
    // map of context level and corresponding animation node
    std::multimap<int, NodePtr> animationMap;
    int lastAnimLevel{0};
    int lastGeomCtxLevel{0};
  };

  // Inlined definitions //////////////////////////////////////////////////////

  inline bool GenerateAnimationSequence::operator()(
      Node &node, TraversalContext &ctx)
  {
    if (node.type() == NodeType::TRANSFORM && !node.hasChild("traversed")) {
      if (node.hasChild("hasAnimations")) {
        auto &nc = node.children();
        for (auto &c : nc) {
          if (c.second->type() == NodeType::ANIMATION) {
            auto animLevel = ctx.level + 1;
            animationMap.insert(std::make_pair(animLevel, c.second));
            lastAnimLevel = animLevel;
          }
        }
        // node.remove("hasAnimations");
        return true;
      } else {
        // this transform  node either has a geometry child or is an animation
        // sequence xfm
        if (node.hasChild("timestep")) {
          return false;
        } else if (node.hasChildren())
          return true;
        else
          return false;
      }
      return true;
    } else if (node.type() == NodeType::GEOMETRY) {
      // check this geometry's ctx level and compare it with all entries of
      // map
      auto geomCtxLevel = ctx.level;
      if (geomCtxLevel >= lastAnimLevel) {
        auto &np = node.parents().front();
        int totalKeyframes = 0;
        // prepare a sequence of animations for this geometry
        for (auto iter = animationMap.begin(); iter != animationMap.end();
             ++iter) {
          if (iter->first <= lastAnimLevel) {
            auto &animNode = *iter->second;
            auto newAnimNode = createNode(animNode.name(), "animation");
            auto &nc = animNode.children();

            for (auto &c : nc)
              newAnimNode->add(c.second);

            node.add(newAnimNode);
            node.createChild("hasAnimations");
            totalKeyframes += iter->second->children().size();
            if (iter->second->hasChild("traversed"))
              totalKeyframes = totalKeyframes - 1;
          }
        }
        np->createChild("totalKeyframes", "int", totalKeyframes);
        lastGeomCtxLevel = geomCtxLevel;
        return false;
      }
      return false;
    }
    return true;
  }

  inline void GenerateAnimationSequence::postChildren(
      Node &node, TraversalContext &ctx)
  {
    if (node.type() == NodeType::TRANSFORM && !node.hasChild("traversed")) {
      for (auto iter = animationMap.begin(); iter != animationMap.end();
           ++iter) {
        if (ctx.level < iter->first) {
          auto copyIt = iter;
          --copyIt;
          lastAnimLevel = copyIt->first;
          animationMap.erase(iter);
        }    
      }
      // if(node.hasChild("hasAnimations"))
      //   node.remove("hasAnimations");
    } else if (node.type() == NodeType::GEOMETRY)
      lastGeomCtxLevel = 0;
    else if (node.type() == NodeType::ANIMATION) {
      if (!node.hasChild("traversed")) {
        auto &np = node.parents().front();
        np->remove(node.name());
        auto &nc = node.children();
        for (auto &c : nc) 
          c.second->createChild("traversed");
        node.createChild("traversed");
      }
    }
  }
  }  // namespace sg{}
} // namespace ospray
