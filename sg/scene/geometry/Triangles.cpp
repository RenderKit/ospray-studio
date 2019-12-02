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

#include "Geometry.h"

namespace ospray::sg {

  struct OSPSG_INTERFACE Triangles : public Geometry
  {
    Triangles();
    virtual ~Triangles() override = default;
  };

  OSP_REGISTER_SG_NODE_NAME(Triangles, geometry_triangles);

  // Triangles definitions ////////////////////////////////////////////////////

  Triangles::Triangles() : Geometry("mesh") {}

}  // namespace ospray::sg
