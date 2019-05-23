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

#define private public
#define protected public
#include "sg/Node.h"
#undef private
#undef protected

using namespace ospray::sg;

#include <type_traits>

SCENARIO("sg::createNode()")
{
  GIVEN("A generic node created from sg::createNode()")
  {
    auto node_ptr = createNode("test_node", "Node", 42, "test documentation");
    auto &node    = *node_ptr;

    THEN("The node's name is correct")
    {
      REQUIRE(node.name() == "test_node");
    }

    THEN("The node's value type is correct")
    {
      REQUIRE(node.valueIsType<int>());
    }

    THEN("The node's value is correct")
    {
      REQUIRE(node.valueAs<int>() == 42);
    }

    THEN("The node's documentation is correct")
    {
      REQUIRE(node.documentation() == "test documentation");
    }
  }

  GIVEN("A specific node type to sg::createNode()")
  {
    auto node_ptr = createNode("test_node", "float", 4.f, "test documentation");
    auto &node    = *node_ptr;

    THEN("The node's type is correct")
    {
      FloatNode *asFloatNode = dynamic_cast<FloatNode*>(node_ptr.get());
      REQUIRE(asFloatNode != nullptr);
    }
  }
}

SCENARIO("sg::Node interface")
{
  GIVEN("A freshly created node")
  {
    auto node_ptr = createNode("test_node");
    auto &node    = *node_ptr;

    TimeStamp whenCreated = node.whenCreated();

    REQUIRE(!node.value().valid());
    REQUIRE(!node.hasChildren());
    REQUIRE(!node.hasParents());

    WHEN("A node is assigned a value")
    {
      node = 1.f;

      THEN("It's value type takes the assigned type and value")
      {
        REQUIRE(node.valueIsType<float>());
        REQUIRE(!node.valueIsType<int>());
        REQUIRE(node.valueAs<float>() == 1.f);
      }

      THEN("The node is marked modified")
      {
        TimeStamp lastModified = node.lastModified();
        REQUIRE(lastModified > whenCreated);
      }

      THEN("Assigning a different value/type changes the Node's stored value")
      {
        node = 1;
        REQUIRE(!node.valueIsType<float>());
        REQUIRE(node.valueIsType<int>());
        REQUIRE(node.valueAs<int>() == 1);
      }
    }

    WHEN("A child is added to the node")
    {
      auto child_ptr = createNode("child");
      auto &child    = *child_ptr;

      TimeStamp initialModified = node.lastModified();

      node.add(child);

      THEN("Node modified values are updated")
      {
        TimeStamp lastModified     = node.lastModified();
        TimeStamp childrenModified = node.childrenLastModified();
        REQUIRE(lastModified > initialModified);
        REQUIRE(childrenModified > initialModified);
        REQUIRE(lastModified > childrenModified);
      }

      THEN("The node's children list updates")
      {
        REQUIRE(node.hasChildren());
        REQUIRE(node.children().size() == 1);
      }

      THEN("The node's parent list does not change")
      {
        REQUIRE(!node.hasParents());
      }

      THEN("The child's parent list updates")
      {
        REQUIRE(child.hasParents());
        REQUIRE(child.parents().size() == 1);
      }

      THEN("The child's child list does not change")
      {
        REQUIRE(!child.hasChildren());
      }

      THEN("The child instance is the same instance of the passed child node")
      {
        REQUIRE(&node["child"] == &child);
        REQUIRE(&node == &(*child.parents().front()));
      }

      THEN("Removing the child reverses Node::add()")
      {
        node.remove("child");

        REQUIRE(!node.hasChildren());
        REQUIRE(!child.hasParents());
      }
    }

    WHEN("A child is added from Node::createChild()")
    {
      auto &child = node.createChild("child", "Node", 42, "docs");

      THEN("The child's name is correct")
      {
        REQUIRE(child.name() == "child");
      }

      THEN("The child's value is correct")
      {
        REQUIRE(child.valueIsType<int>());
        REQUIRE(child.valueAs<int>() == 42);
      }

      THEN("The child's documentation is correct")
      {
        REQUIRE(child.documentation() == "docs");
      }

      THEN("The child's structural relationship is correct")
      {
        REQUIRE(node.hasChildren());
        REQUIRE(!node.hasParents());
        REQUIRE(child.hasParents());
        REQUIRE(!child.hasChildren());
      }

      THEN("The child's instance is correctly returned from createChild()")
      {
        REQUIRE(&node["child"] == &child);
        REQUIRE(&node == &(*child.parents().front()));
      }

      THEN("Removing the child reverses createChild()")
      {
        node.remove("child");

        REQUIRE(!node.hasChildren());
        REQUIRE(!child.hasParents());
      }
    }
  }
}

SCENARIO("sg::Node_T<> interface")
{
  GIVEN("A freshly created sg::FloatNode")
  {
    auto floatNode_ptr = createNodeAs<FloatNode>("test float", "float", "test");
    auto &floatNode    = *floatNode_ptr;

    floatNode = 1.f;

    REQUIRE(floatNode.valueIsType<float>());
    REQUIRE(!floatNode.valueIsType<int>());

    THEN("The node's value() return type is correct")
    {
      auto value = floatNode.value();

      REQUIRE(value == 1.f);
      static_assert(std::is_same<decltype(value), float>::value);
    }

    THEN("Value type is converted on assignment")
    {
      floatNode = 2;

      auto value = floatNode.value();

      REQUIRE(value == 2.f);
      static_assert(std::is_same<decltype(value), float>::value);
    }

    THEN("Value type conversion happens on query")
    {
      int value = floatNode.value();
      REQUIRE(value == 1);
    }

    THEN("The node implicitly converts to the underlying value type")
    {
      float value = floatNode;
      REQUIRE(value == 1.f);
    }
  }
}
