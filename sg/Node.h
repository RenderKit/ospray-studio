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

#pragma once

#include "Visitor.h"
// stl
#include <map>
#include <memory>
#include <vector>
// ospcommon
#include "ospcommon/utility/Any.h"
#include "ospcommon/utility/TimeStamp.h"
#include "ospcommon/vec.h"
// ospray
#include "ospray/ospray.h"

#ifndef OSPSG_INTERFACE
#ifdef _WIN32
#ifdef ospray_sg_EXPORTS
#define OSPSG_INTERFACE __declspec(dllexport)
#else
#define OSPSG_INTERFACE __declspec(dllimport)
#endif
#define OSPSG_DLLEXPORT __declspec(dllexport)
#else
#define OSPSG_INTERFACE
#define OSPSG_DLLEXPORT
#endif
#endif

namespace ospray {
  namespace sg {

    using namespace ospcommon;

    using Any       = utility::Any;
    using TimeStamp = utility::TimeStamp;

    ///////////////////////////////////////////////////////////////////////////
    // Generic Node class definition //////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    struct Node;
    using NodePtr = std::shared_ptr<Node>;

    struct OSPSG_INTERFACE Node : public std::enable_shared_from_this<Node>
    {
      Node();
      virtual ~Node();

      // NOTE: Nodes are not copyable nor movable! The operator=() will be used
      //       to assign a Node's _value_, which is different than the
      //       parent/child structure of the Node itself.
      Node(const Node &) = delete;
      Node(Node &&)      = delete;
      Node &operator=(const Node &) = delete;
      Node &operator=(Node &&) = delete;

      virtual std::string toString() const;

      template <typename T>
      std::shared_ptr<T> nodeAs();  // static cast (faster, but not safe!)

      template <typename T>
      std::shared_ptr<T> tryNodeAs();  // dynamic cast (slower, but can check)

      // Properties ///////////////////////////////////////////////////////////

      std::string name() const;
      std::string type() const;
      std::string documentation() const;

      size_t uniqueID() const;

      // Node stored value (data) interface ///////////////////////////////////

      Any value();

      template <typename T>
      T &valueAs();

      template <typename T>
      const T &valueAs() const;

      template <typename T>
      bool valueIsType() const;

      template <typename T>
      void setValue(T val);

      void operator=(Any val);

      // Update detection interface ///////////////////////////////////////////

      TimeStamp whenCreated() const;
      TimeStamp lastModified() const;
      TimeStamp lastCommitted() const;
      TimeStamp childrenLastModified() const;

      // Parent-child structual interface /////////////////////////////////////

      using NodeLink = std::pair<std::string, NodePtr>;

      // Children //

      const std::map<std::string, NodePtr> &children() const;

      bool hasChildren() const;

      bool hasChild(const std::string &name) const;

      Node &child(const std::string &name) const;
      Node &operator[](const std::string &c) const;

      // Parents //

      const std::vector<NodePtr> &parents() const;

      bool hasParents() const;

      // Structural Changes (add/remove children) //

      void add(Node &node);
      void add(Node &node, const std::string &name);

      void add(NodePtr node);
      void add(NodePtr node, const std::string &name);

      void remove(Node &node);
      void remove(NodePtr node);
      void remove(const std::string &name);

      template <typename T>
      Node &createChildWithValue(const std::string &name,
                                 const std::string &type,
                                 const T &t);

      Node &createChild(std::string name,
                        std::string type          = "Node",
                        Any value                 = Any(),
                        std::string documentation = "");

      template <typename NODE_T>
      NODE_T &createChild(std::string name,
                          std::string type          = "Node",
                          std::string documentation = "");

      // Traversal interface //////////////////////////////////////////////////

      //! Use a custom provided node visitor to visit each node
      template <typename VISITOR_T, typename = is_valid_visitor_t<VISITOR_T>>
      void traverse(VISITOR_T &&visitor, TraversalContext &ctx);

      //! Helper overload to traverse with a default constructed TravesalContext
      template <typename VISITOR_T, typename = is_valid_visitor_t<VISITOR_T>>
      void traverse(VISITOR_T &&visitor);

      // Private Members //////////////////////////////////////////////////////

     private:
      struct
      {
        std::string name;
        std::string type;
        std::string documentation;

        Any value;

        std::map<std::string, NodePtr> children;
        std::vector<NodePtr> parents;

        TimeStamp whenCreated;
        TimeStamp lastModified;
        TimeStamp childrenMTime;
        TimeStamp lastCommitted;
        TimeStamp lastVerified;
      } properties;

      void removeFromParentList(Node &node);

      void markAsModified();
      void setChildrenModified(TimeStamp t);

      void setName(const std::string &v);
      void setType(const std::string &v);
      void setDocumentation(const std::string &s);

      friend NodePtr createNode(std::string, std::string, Any, std::string);
    };

    ///////////////////////////////////////////////////////////////////////////
    // Nodes with a strongly-typed value //////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    template <typename VALUE_T>
    struct Node_T : public Node
    {
      Node_T()                   = default;
      virtual ~Node_T() override = default;

      const VALUE_T &value() const;

      template <typename OT>
      void operator=(OT &&val);
    };

    // Pre-defined parameter nodes ////////////////////////////////////////////

    using StringNode = Node_T<std::string>;

    using BoolNode = Node_T<bool>;

    using FloatNode = Node_T<float>;
    using Vec2fNode = Node_T<vec2f>;
    using Vec3fNode = Node_T<vec3f>;
    using Vec4fNode = Node_T<vec4f>;

    using IntNode   = Node_T<int>;
    using Vec2iNode = Node_T<vec2i>;
    using Vec3iNode = Node_T<vec3i>;

    using VoidPtrNode = Node_T<void *>;

    ///////////////////////////////////////////////////////////////////////////
    // Main Node factory function /////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    OSPSG_INTERFACE NodePtr createNode(std::string name,
                                       std::string type          = "Node",
                                       Any var                   = Any(),
                                       std::string documentation = "");

    template <typename NODE_T, typename... Args>
    inline std::shared_ptr<NODE_T> make_node(Args &&... args)
    {
      return std::make_shared<NODE_T>(std::forward<Args>(args)...);
    }

    ///////////////////////////////////////////////////////////////////////////
    // Node factory function registration /////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

#define OSP_REGISTER_SG_NODE_NAME(InternalClassName, Name)                     \
  extern "C" OSPSG_DLLEXPORT ospray::sg::Node *ospray_create_sg_node__##Name() \
  {                                                                            \
    return new InternalClassName;                                              \
  }                                                                            \
  /* Extra declaration to avoid "extra ;" pedantic warnings */                 \
  ospray::sg::Node *ospray_create_sg_node__##Name()

#define OSP_REGISTER_SG_NODE(InternalClassName) \
  OSP_REGISTER_SG_NODE_NAME(InternalClassName, InternalClassName)

  }  // namespace sg
}  // namespace ospray

#include "Node.inl"
