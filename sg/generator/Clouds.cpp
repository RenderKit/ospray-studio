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

#include "Generator.h"
// std
#include <random>
// rkcommon
#include "rkcommon/tasking/parallel_for.h"

namespace ospray::sg {

  struct Clouds : public Generator
  {
    Clouds();
    ~Clouds() override = default;

    void generateData() override;
  };

  OSP_REGISTER_SG_NODE_NAME(Clouds, generator_clouds);

  // Clouds definitions ////////////////////////////////////////////////////////

  Clouds::Clouds()
  {
    auto &parameters = child("parameters");
    /*

    parameters.createChild("dimensions", "vec3i", vec3i(128));
    parameters.createChild("gridOrigin", "vec3f", vec3f(-1.f));
    parameters.createChild("gridSpacing", "vec3f", vec3f(2.f / 100));
    */
  }

  void Clouds::generateData()
  {
    remove("clouds");
    /*
    remove("wavelet");

    // Create voxel data

    auto &parameters = child("parameters");

    auto dimensions  = parameters["dimensions"].valueAs<vec3i>();
    auto gridOrigin  = parameters["gridOrigin"].valueAs<vec3f>();
    auto gridSpacing = parameters["gridSpacing"].valueAs<vec3f>();

    std::vector<float> voxels(dimensions.long_product());

    auto transformLocalToObject = [&](const vec3f &localCoordinates) {
      return gridOrigin + localCoordinates * gridSpacing;
    };

    tasking::parallel_for(dimensions.z, [&](int z) {
      for (size_t y = 0; y < dimensions.y; y++) {
        for (size_t x = 0; x < dimensions.x; x++) {
          size_t index = z * dimensions.y * dimensions.x + y * dimensions.x + x;
          vec3f objectCoordinates = transformLocalToObject(vec3f(x, y, z));
          voxels[index]           = getWaveletValue(objectCoordinates);
        }
      }
    });

    // Create sg subtree
    auto &tf     = createChild("transferFunction", "transfer_function_jet");
    auto &volume = tf.createChild("wavelet", "volume_structured");     
    volume["voxelType"]   = int(OSP_FLOAT);
    volume["gridOrigin"]  = gridOrigin;
    volume["gridSpacing"] = gridSpacing;
    volume["dimensions"]  = dimensions;
    volume.createChildData("data", dimensions, 0, voxels.data());
    */
  }

}  // namespace ospray::sg
