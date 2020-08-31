// ======================================================================== //
// Copyright 2009-2020 Intel Corporation                                    //
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

#include "World.h"
#include "lights/Lights.h"

namespace ospray {
  namespace sg {

  World::World()
  {
    setHandle(cpp::World());
    createChild("lights", "lights");
  }

  void World::preCommit() {    
    auto ospWorld = valueAs<cpp::World>();
    auto &lights = childAs<sg::Lights>("lights");
    std::vector<cpp::Light> lightObjects;

    for (auto &name : lights.lightNames) {
      auto &l = lights.child(name).valueAs<cpp::Light>();
      lightObjects.push_back(l);
    }

    ospWorld.setParam("light", cpp::CopiedData(lightObjects));
  }

  void World::postCommit() {
    auto ospWorld = valueAs<cpp::World>();
    ospWorld.commit();
  }

  OSP_REGISTER_SG_NODE_NAME(World, world);

  }  // namespace sg
} // namespace ospray
