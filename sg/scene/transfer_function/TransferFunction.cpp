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

#include "TransferFunction.h"

namespace ospray {
  namespace sg {

  TransferFunction::TransferFunction(const std::string &osp_type)
  {
    setValue(cpp::TransferFunction(osp_type));

    createChild("valueRange", "vec2f", vec2f(0.f, 1.f));
    std::vector<vec3f> colors = {vec3f(0.f), vec3f(1.f)};

    // createChild("valueRange", "vec2f", vec2f(-10000.f, 10000.f));

    // std::vector<vec3f> colors = {vec3f(1.0f, 0.0f, 0.0f),
    //                              vec3f(0.0f, 1.0f, 0.0f)};

    createChildData("color", colors);
    std::vector<float> opacities = {0.f, 1.f};
    createChildData("opacity", opacities);
  }

  }  // namespace sg
} // namespace ospray
