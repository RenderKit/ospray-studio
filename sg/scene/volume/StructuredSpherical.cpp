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

#include "Volume.h"
#include "StructuredSpherical.h"

namespace ospray {
  namespace sg {

  // StructuredSpherical definitions /////////////////////////////////////////////

  StructuredSpherical::StructuredSpherical() : Volume("structuredSpherical")
  {
    createChild("voxelType", "int");
    createChild("gridOrigin", "vec3f");
    createChild("gridSpacing", "vec3f");
    createChild("dimensions", "vec3i");
  }

    ///! \brief file name of the xml doc when the node was loaded from xml
  /*! \detailed we need this to properly resolve relative file names */
  void StructuredSpherical::load(const FileName &fileNameAbs)
  {
    // the following hard coded volume properties should exist either in volume file headers or,
    // xml kind supporting docs with binary volume data
    createChild("voxelType", "int", int(OSP_FLOAT));
    createChild("dimensions", "vec3i", vec3i(180, 180, 180));
    createChild("gridOrigin", "vec3f", vec3f(0));
    createChild("gridSpacing", "vec3f", vec3f(1, 1, 1));

    auto &volume = valueAs<cpp::Volume>();

    auto &dimensions = child("dimensions").valueAs<vec3i>();
    if (dimensions.x <= 0 || dimensions.y <= 0 || dimensions.z <= 0) {
      throw std::runtime_error(
          "StructuredRegular::render(): "
          "invalid volume dimensions");
    }

    std::vector<float> voxels(dimensions.product());

    std::ifstream input(fileNameAbs.c_str(), std::ios::binary);

    if (!input) {
      throw std::runtime_error("error opening raw volume file");
    }

    input.read((char *)voxels.data(), dimensions.product() * sizeof(float));

    if (!input.good()) {
      throw std::runtime_error("error reading raw volume file");
    }
     
    createChildData("data", dimensions, OSP_FLOAT, voxels.data());

    fileLoaded = true;
  }

   OSP_REGISTER_SG_NODE_NAME(StructuredSpherical, structuredSpherical);

  }  // namespace sg
} // namespace ospray
