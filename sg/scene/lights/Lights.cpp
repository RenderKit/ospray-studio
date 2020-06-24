// ======================================================================== //
// Copyright 2020 Intel Corporation                                         //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

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

    bool Lights::addLight(std::string name, std::string lightType)
    {
      auto found = std::find(lightNames.begin(), lightNames.end(), name);
      if (found == lightNames.end()) {
        lightNames.push_back(name);
        createChild(name, lightType);
      } else {
        return false;
      }
      return true;
    }

    bool Lights::removeLight(std::string name)
    {
      auto found = std::find(lightNames.begin(), lightNames.end(), name);
      if (found == lightNames.end()) {
        return false;
      } else {
        remove(name);
        lightNames.erase(found);
      }
      markAsModified();
      return true;
    }

  }  // namespace sg
}  // namespace ospray
