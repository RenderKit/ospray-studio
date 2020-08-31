// ======================================================================== //
// Copyright 2020 Intel Corporation                                    //
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

#include "Light.h"

namespace ospray {
  namespace sg {

  struct OSPSG_INTERFACE QuadLight : public Light
  {
    QuadLight();
    virtual ~QuadLight() override = default;
  };

  OSP_REGISTER_SG_NODE_NAME(QuadLight, quad);

  // QuadLight definitions /////////////////////////////////////////////

  QuadLight::QuadLight() : Light("quad")
  {
    createChild("position", "vec3f", vec3f(0.f));
    createChild("edge1", "vec3f", vec3f(1.f, 0.f, 0.f));
    createChild("edge2", "vec3f", vec3f(0.f, 1.f, 0.f));
  }

  }  // namespace sg
} // namespace ospray
