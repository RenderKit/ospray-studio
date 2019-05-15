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

#include "catch/catch.hpp"

#include "sg2/Node.h"
using namespace ospray::sg;

TEST_CASE("Node interface", "")
{
  auto node_ptr = createNode("test_node");
  auto &node    = *node_ptr;

  TimeStamp whenCreated = node.whenCreated();

  SECTION("Node values")
  {
    REQUIRE(!node.value().valid());

    node = 1.f; // set as float

    REQUIRE(node.valueIsType<float>());
    REQUIRE(!node.valueIsType<int>());
    REQUIRE(node.valueAs<float>() == 1.f);

    TimeStamp lastModified = node.lastModified();
    REQUIRE(lastModified > whenCreated);

    node = 1; // set as int

    REQUIRE(!node.valueIsType<float>());
    REQUIRE(node.valueIsType<int>());
    REQUIRE(node.valueAs<int>() == 1);
    REQUIRE(lastModified < node.lastModified());
  }
}
