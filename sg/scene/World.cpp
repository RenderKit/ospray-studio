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

namespace ospray::sg {

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
      std::cout << "world found light '" << name << "'" << std::endl;
      auto &l = lights.child(name).valueAs<cpp::Light>();
      lightObjects.push_back(l);
    }
    std::cout << lightObjects.size() << " light"
              << ((lightObjects.size() > 1) ? "s" : "") << std::endl;

    ospWorld.setParam("light", cpp::Data(lightObjects));
  }

  void World::postCommit() {
    auto ospWorld = valueAs<cpp::World>();
    ospWorld.commit();
  }

  OSP_REGISTER_SG_NODE_NAME(World, world);

}  // namespace ospray::sg
