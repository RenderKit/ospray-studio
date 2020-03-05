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

#include "../Material.h"

namespace ospray::sg {

  struct OSPSG_INTERFACE MaterialOBJ : public Material
  {
    MaterialOBJ();
    ~MaterialOBJ() override = default;
  };

  OSP_REGISTER_SG_NODE_NAME(MaterialOBJ, material_obj);

  // MaterialOBJ definitions //////////////////////////////////////////////////

  MaterialOBJ::MaterialOBJ() : Material("obj")
  {
    createChild("kd", "rgb", vec3f(0.f));
    createChild("ks", "rgb", vec3f(0.8f));
    createChild("ns", "float", 0.f);
  }

}  // namespace ospray::sg
