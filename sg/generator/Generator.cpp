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

namespace ospray {
  namespace sg {

  Generator::Generator()
  {
    createChild("parameters");
  }

  NodeType Generator::type() const
  {
    return NodeType::GENERATOR;
  }

  void Generator::generateDataAndMat(std::shared_ptr<sg::MaterialRegistry> materialRegistry)
  {
  }

  void Generator::generateData()
  {
  }

  }  // namespace sg
} // namespace ospray
