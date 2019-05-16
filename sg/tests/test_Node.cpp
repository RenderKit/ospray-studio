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

#include "sg/Node.h"
using namespace ospray::sg;

#include <type_traits>

TEST_CASE("Test sg::createNode()")
{
  auto node_ptr = createNode("test_node", "Node", 42, "test documentation");
  auto &node    = *node_ptr;

  REQUIRE(node.name() == "test_node");
  REQUIRE(node.valueIsType<int>());
  REQUIRE(node.valueAs<int>() == 42);
  REQUIRE(node.documentation() == "test documentation");
}

TEST_CASE("Test sg::Node interface")
{
  auto node_ptr = createNode("test_node");
  auto &node    = *node_ptr;

  TimeStamp whenCreated = node.whenCreated();

  SECTION("Node values")
  {
    REQUIRE(!node.value().valid());

    node = 1.f;  // set as float

    REQUIRE(node.valueIsType<float>());
    REQUIRE(!node.valueIsType<int>());
    REQUIRE(node.valueAs<float>() == 1.f);

    TimeStamp lastModified = node.lastModified();
    REQUIRE(lastModified > whenCreated);

    node = 1;  // set as int

    REQUIRE(!node.valueIsType<float>());
    REQUIRE(node.valueIsType<int>());
    REQUIRE(node.valueAs<int>() == 1);
    REQUIRE(lastModified < node.lastModified());
  }

  SECTION("Node parent/child relationships")
  {
    REQUIRE(!node.hasChildren());
    REQUIRE(!node.hasParents());

    SECTION("Add child from existing node")
    {
      auto child_ptr = createNode("child");
      auto &child    = *child_ptr;

      node.add(child);

      REQUIRE(node.hasChildren());
      REQUIRE(!node.hasParents());
      REQUIRE(child.hasParents());
      REQUIRE(!child.hasChildren());

      REQUIRE(&node["child"] == &child);
      REQUIRE(&node == &(*child.parents().front()));

      node.remove("child");

      REQUIRE(!node.hasChildren());
      REQUIRE(!child.hasParents());
    }

    SECTION("Add child from createChild()")
    {
      auto &child = node.createChild("child", "Node", 42, "docs");

      REQUIRE(child.name() == "child");
      REQUIRE(child.valueIsType<int>());
      REQUIRE(child.valueAs<int>() == 42);
      REQUIRE(child.documentation() == "docs");

      REQUIRE(node.hasChildren());
      REQUIRE(!node.hasParents());
      REQUIRE(child.hasParents());
      REQUIRE(!child.hasChildren());

      REQUIRE(&node["child"] == &child);
      REQUIRE(&node == &(*child.parents().front()));

      node.remove("child");

      REQUIRE(!node.hasChildren());
      REQUIRE(!child.hasParents());
    }
  }
}

TEST_CASE("Test sg::Node_T<> interface")
{
  auto floatNode_ptr = make_node<FloatNode>();
  auto &floatNode    = *floatNode_ptr;

  floatNode = 1.f;

  REQUIRE(floatNode.valueIsType<float>());
  REQUIRE(!floatNode.valueIsType<int>());

  SECTION("Value type correctness")
  {
    auto value = floatNode.value();

    REQUIRE(value == 1.f);
    static_assert(std::is_same<decltype(value), float>::value);
  }

  SECTION("Value type conversion from assignment")
  {
    floatNode = 2;

    auto value = floatNode.value();

    REQUIRE(value == 2.f);
    static_assert(std::is_same<decltype(value), float>::value);
  }

  SECTION("Value type conversion from query")
  {
    int value = floatNode.value();
    REQUIRE(value == 1);
  }
}
