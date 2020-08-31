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

  struct OSPSG_INTERFACE Jet : public TransferFunction
  {
    Jet();
    virtual ~Jet() override = default;
  };

  OSP_REGISTER_SG_NODE_NAME(Jet, transfer_function_jet);

  // Jet definitions //////////////////////////////////////////////////////////

  Jet::Jet() : TransferFunction("piecewiseLinear")
  {
    std::vector<vec3f> colors;
    colors.emplace_back(0, 0, 0.562493);
    colors.emplace_back(0, 0, 1);
    colors.emplace_back(0, 1, 1);
    colors.emplace_back(0.500008, 1, 0.500008);
    colors.emplace_back(1, 1, 0);
    colors.emplace_back(1, 0, 0);
    colors.emplace_back(0.500008, 0, 0);

    createChildData("color", colors);
  }

  }  // namespace sg
} // namespace ospray
