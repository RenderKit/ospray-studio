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
      bool addLight(std::string name, std::string lightType);
      bool removeLight(std::string name);

     // protected:
      std::vector<std::string> lightNames;
    };

  }  // namespace sg
}  // namespace ospray
