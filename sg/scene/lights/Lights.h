// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../../Node.h"

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
    struct OSPSG_INTERFACE Lights : public Node
    {
      Lights();
      ~Lights() override = default;
      NodeType type() const override;
      bool lightExists(std::string name);
      bool addLight(std::string name, std::string lightType);
      bool removeLight(std::string name);

     // protected:
      std::vector<std::string> lightNames;
    };

  }  // namespace sg
}  // namespace ospray
