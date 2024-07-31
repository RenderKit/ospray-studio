// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "sg/Node.h"
#include "sg/scene/World.h"

namespace ospray {
namespace sg {

/**
 * LightsManager node - manages lights in the world
 *
 * This node maintains a list of all lights in the world. It holds a list of
 * Light nodes which is passed to the world on commit. This list lets us
 * add/remove lights without adding unrelated functionality to the World
 * node
 */
struct OSPSG_INTERFACE LightsManager
    : public OSPNode<cpp::Light, NodeType::LIGHTS>
{
  LightsManager();
  ~LightsManager() override = default;
  NodeType type() const override;
  bool lightExists(std::string name);
  bool addLight(std::string name, std::string lightType);
  bool addLight(NodePtr light);
  void addLights(std::vector<NodePtr> &lights);
  void addGroupLight(NodePtr light);
  void addGroupLights(std::vector<NodePtr> &lights);
  bool removeLight(std::string name);
  void clear();
  NodePtr getLight(std::string name);

  inline bool hasDefaultLight()
  {
    return hasChild("default-ambient");
  }

  void updateWorld(World &world);
  bool rmDefaultLight{true};

 protected:
  std::vector<std::string> lightNames;
  std::vector<cpp::Light> cppWorldLightObjects;

  virtual void preCommit() override;
  virtual void postCommit() override;

 private:
};

} // namespace sg
} // namespace ospray
