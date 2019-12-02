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

  struct RandomSpheres : public Generator
  {
    RandomSpheres()           = default;
    ~RandomSpheres() override = default;

    void generateData() override;
  };

  OSP_REGISTER_SG_NODE_NAME(RandomSpheres, generator_random_spheres);

  // RandomSpheres definitions ////////////////////////////////////////////////

  void RandomSpheres::generateData()
  {
    remove("spheres");

    const int numSpheres = 1e6;
    const float radius   = 0.002f;

    auto &spheres = createChild("spheres", "geometry_spheres");

    std::mt19937 rng(0);
    std::uniform_real_distribution<float> dist(-1.f + radius, 1.f - radius);

    std::vector<vec3f> centers;

    for (int i = 0; i < numSpheres; ++i)
      centers.push_back(vec3f(dist(rng), dist(rng), dist(rng)));

    spheres.createChildData("sphere.position", centers);
    spheres.child("radius") = radius;
  }

}  // namespace ospray::sg
