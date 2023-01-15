// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "catch/catch.hpp"

#define protected public
#include "sg/Node.h"
#undef protected

using namespace ospray::sg;

#include <type_traits>

SCENARIO("sg::createNode()")
{
  GIVEN("A generic node created from sg::createNode()")
  {
    auto node_ptr = createNode("test_node", "Node", "test description", 42);
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

    THEN("The node's type is NodeType::GENERIC")
    {
      REQUIRE(node.type() == NodeType::GENERIC);
    }

    THEN("The node's description is correct")
    {
      REQUIRE(node.description() == "test description");
    }
  }

  GIVEN("A specific node type to sg::createNode()")
  {
    auto node_ptr = createNode("test_node", "float", "test description", 4.f);
    auto &node    = *node_ptr;

    THEN("The node's type is correct")
    {
      FloatNode *asFloatNode = dynamic_cast<FloatNode *>(node_ptr.get());
      REQUIRE(asFloatNode != nullptr);
      REQUIRE(node.valueAs<float>() == asFloatNode->value());
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
      auto &child = node.createChild("child", "Node", "docs", 42);

      THEN("The child's name is correct")
      {
        REQUIRE(child.name() == "child");
      }

      THEN("The child's value is correct")
      {
        REQUIRE(child.valueIsType<int>());
        REQUIRE(child.valueAs<int>() == 42);
      }

      THEN("The child's description is correct")
      {
        REQUIRE(child.description() == "docs");
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

SCENARIO("sg::Node structural updates")
{
  GIVEN("A node that has a child and is committed")
  {
    auto parent_ptr = createNode("parent_node");
    auto &parent    = *parent_ptr;
    auto &child     = parent.createChild("child_node");

    parent.commit();

    TimeStamp initialModifiedParent = parent.lastModified();
    TimeStamp initialModifiedChild  = child.lastModified();

    WHEN("Assigning a value to the parent")
    {
      parent = 4;

      THEN("Time stamps are correct")
      {
        REQUIRE(parent.lastModified() > parent.lastCommitted());
        REQUIRE(parent.lastModified() > initialModifiedParent);
        REQUIRE(parent.lastModified() > initialModifiedChild);
        REQUIRE(parent.childrenLastModified() < parent.lastModified());
      }
    }

    WHEN("Assigning a value to the child")
    {
      child = 5;

      THEN("Time stamps are correct")
      {
        REQUIRE(parent.lastModified() == initialModifiedParent);
        REQUIRE(parent.childrenLastModified() > initialModifiedParent);
        REQUIRE(parent.lastModified() < parent.childrenLastModified());
      }

      WHEN("...and the parent is committed")
      {
        auto parentPreCommit = parent.lastCommitted();
        parent.commit();

        THEN("Time stamps are correct")
        {
          REQUIRE(parentPreCommit < parent.lastCommitted());
          REQUIRE(parent.lastModified() < parent.lastCommitted());
          REQUIRE(child.lastModified() < child.lastCommitted());
        }
      }
      WHEN("...and the parent is NOT committed") 
      {
        THEN("Time stamps are correct")
        {
          REQUIRE(child.lastModified() > initialModifiedChild);
          REQUIRE(child.lastModified() > parent.lastModified());
          REQUIRE(child.lastModified() > parent.lastCommitted());
          REQUIRE(child.lastModified() > child.lastCommitted());
        }
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
    REQUIRE(floatNode.type() == NodeType::PARAMETER);

    THEN("The node's value() return type is correct")
    {
      auto value = floatNode.value();

      REQUIRE(value == 1.f);
      static_assert(std::is_same<decltype(value), float>::value,
        "is_same: node value types don't match");
    }

    THEN("Value type is converted on assignment")
    {
      floatNode = 2;

      auto value = floatNode.value();

      REQUIRE(value == 2.f);
      static_assert(std::is_same<decltype(value), float>::value,
        "is_same: node value types don't match");
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
