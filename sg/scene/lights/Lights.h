// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../../Node.h"
#include "../World.h"

namespace ospray {
  namespace sg {

    /**
     * Lights node - manages lights in the world
     *
     * This node maintains a list of all lights in the world. It holds a list of
     * Light nodes which is passed to the world on commit. This list lets us
     * add/remove lights without adding unrelated functionality to the World
     * node
     */
    struct OSPSG_INTERFACE Lights : public OSPNode<cpp::Light, NodeType::LIGHTS>
    {
      Lights();
      ~Lights() override = default;
      NodeType type() const override;
      bool lightExists(std::string name);
      bool addLight(std::string name, std::string lightType);
      bool addLight(NodePtr light);
      void addLights(std::vector<NodePtr> &lights);
      bool removeLight(std::string name);

      void updateWorld(World &world);

     // protected:
      std::vector<std::string> lightNames;
      std::vector<cpp::Light> cppLightObjects = {};

      bool isStubborn{false}; // XXX HACK, until markAsModified is reliable

      virtual void preCommit() override;
      virtual void postCommit() override;
    };

  }  // namespace sg
}  // namespace ospray