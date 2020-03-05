// ======================================================================== //
// Copyright 2009-2019 Intel Corporation                                    //
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

#include "Generator.h"
// std
#include <random>

namespace ospray::sg {

  struct Sphere : public Generator
  {
    Sphere()           = default;
    ~Sphere() override = default;

    void generateData() override;
  };

  OSP_REGISTER_SG_NODE_NAME(Sphere, generator_sphere);

  // Sphere definitions ////////////////////////////////////////////////

  void Sphere::generateData()
  {
    auto &spheres = createChild("geometry", "geometry_spheres");
    vec3f center = vec3f{1.f, 1.f, 1.f};
    float radius = 1.f;
    spheres.createChildData("sphere.position", center);
    spheres.child("radius") = radius;
  }

}  // namespace ospray::sg
