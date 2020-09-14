// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Lights.h"

namespace ospray {
  namespace sg {

    OSP_REGISTER_SG_NODE_NAME(Lights, lights);

    Lights::Lights()
    {
      lightNames.push_back("ambient");
      createChild("ambient", "ambient");
    }

    NodeType Lights::type() const
    {
      return NodeType::LIGHTS;
    }

    bool Lights::lightExists(std::string name)
    {
      auto found = std::find(lightNames.begin(), lightNames.end(), name);
      return (found != lightNames.end());
    }

    bool Lights::addLight(std::string name, std::string lightType)
    {
      if (name == "" || lightExists(name))
        return false;

      lightNames.push_back(name);
      createChild(name, lightType);
      return true;
    }

    bool Lights::removeLight(std::string name)
    {
      if (name == "" || !lightExists(name))
        return false;

      remove(name);
      auto found = std::find(lightNames.begin(), lightNames.end(), name);
      lightNames.erase(found);

      markAsModified();
      return true;
    }

  }  // namespace sg
}  // namespace ospray
