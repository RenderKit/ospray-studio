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

namespace ospray::sg {

  struct OSPSG_INTERFACE SunSky : public Light
  {
    SunSky();
    virtual ~SunSky() override = default;

    protected:
    void preCommit() override;
  };

  OSP_REGISTER_SG_NODE_NAME(SunSky, sunSky);

  // Sunsky definitions //////////////////////////////////////////////////////

  SunSky::SunSky() : Light("sunSky")
  {
    createChild("up", "vec3f", vec3f(0.f, 1.f, 0.f));
    createChild("azimuth", "float", 0.f);
    createChild("elevation", "float", 90.f);
    createChild("direction", "vec3f", vec3f(0.f, 0.f, 1.f));
    createChild("albedo", "float", 0.18f);
    createChild("turbidity", "float", 3.f);
    createChild("color", "vec3f", vec3f(1.f));
    createChild("intensity", "float", 1.f);
  }

  void SunSky::preCommit()
  {
    const float azimuth = child("azimuth").valueAs<float>() * M_PI/180.f;
    const float elevation = child("elevation").valueAs<float>() * M_PI/180.f;

    vec3f direction;
    direction.x = -cos(azimuth) * cos(elevation);
    direction.y = -sin(elevation);
    direction.z = -sin(azimuth) * cos(elevation);

    // this overwrites the "direction" child parameters, making that UI element
    // not directly useable...
    auto &directionNode = child("direction");
    directionNode.setValue(direction);

    Light::preCommit();
  }

}  // namespace ospray::sg
