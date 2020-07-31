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
// rkcommon
#include "rkcommon/containers/FlatMap.h"
#include "rkcommon/math/box.h"
#include "rkcommon/math/vec.h"
#include "rkcommon/utility/Any.h"
#include "rkcommon/utility/TimeStamp.h"
#include "rkcommon/math/AffineSpace.h"
// ospray
#include "ospray/ospray_cpp.h"
// ospray_sg
#include "NodeType.h"

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

  using namespace rkcommon;
  using namespace rkcommon::math;

  using Any       = utility::Any;
  using TimeStamp = utility::TimeStamp;

  template <typename K, typename V>
  using FlatMap = rkcommon::containers::FlatMap<K, V>;

  using rgb  = vec3f;
  using rgba = vec4f;

  /////////////////////////////////////////////////////////////////////////////
  // Generic Node class definition ////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  struct Node;
  using NodePtr = std::shared_ptr<Node>;

  struct Data;

  struct OSPSG_INTERFACE Node : public std::enable_shared_from_this<Node>
  {
    Node();
    virtual ~Node() = default;

    // NOTE: Nodes are not copyable nor movable! The operator=() will be used
    //       to assign a Node's _value_, which is different than the
    //       parent/child structure of the Node itself.
    Node(const Node &) = delete;
    Node(Node &&)      = delete;

    Node &operator=(const Node &) = delete;
    Node &operator=(Node &&) = delete;

    template <typename T>
    std::shared_ptr<T> nodeAs();  // static cast (faster, but not safe!)

    template <typename T>
    std::shared_ptr<T> tryNodeAs();  // dynamic cast (slower, but can check)

    // Properties /////////////////////////////////////////////////////////////

    std::string name() const;
    virtual NodeType type() const;
    std::string subType() const;
    std::string description() const;

    size_t uniqueID() const;

    // Node stored value (data) interface /////////////////////////////////////

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

    // Parent-child structural interface ///////////////////////////////////////

    using NodeLink = std::pair<std::string, NodePtr>;

    // Children //

    const FlatMap<std::string, NodePtr> &children() const;

    bool hasChildren() const;

    bool hasChild(const std::string &name) const;

    Node &child(const std::string &name);
    Node &operator[](const std::string &c);

    template <typename NODE_T>
    NODE_T &childAs(const std::string &name);

    template <typename NODE_T>
    std::shared_ptr<NODE_T> childNodeAs(const std::string &name);

    // Parents //

    const std::vector<Node *> &parents() const;

    bool hasParents() const;

    // Structural Changes (add/remove children) //

    void add(Node &node);
    void add(Node &node, const std::string &name);

    void add(NodePtr node);
    void add(NodePtr node, const std::string &name);

    void remove(Node &node);
    void remove(NodePtr node);
    void remove(const std::string &name);

    void removeAllParents();
    void removeAllChildren();

    template <typename... Args>
    Node &createChild(Args &&... args);

    template <typename NODE_T, typename... Args>
    NODE_T &createChildAs(Args &&... args);

    template <typename... Args>
    void createChildData(std::string name, Args &&... args);

    // Public method, where anyChildModified is protected
    inline bool isModified()
    {
      return anyChildModified();
    }

    // Traversal interface ////////////////////////////////////////////////////

    //! Helper overload to traverse with a default constructed TravesalContext
    template <typename VISITOR_T>
    void traverse(VISITOR_T &&visitor);

    template <typename VISITOR_T, typename... Args>
    void traverse(Args &&... args);

    void commit();
    void render();
    box3f bounds();

    virtual void setOSPRayParam(std::string param, OSPObject handle);

    // UI generation //////////////////////////////////////////////////////////

    // limits on UI elements
    Any min() const;

    Any max() const;

    template <typename T>
    const T &minAs() const;

    template <typename T>
    const T &maxAs() const;

    void setMinMax(const Any &minVal, const Any &maxVal);

    bool hasMinMax() const;

    // prevent user adjustment to this Node *via the UI*
    bool readOnly() const;
    void setReadOnly();

   protected:
    virtual void preCommit();
    virtual void postCommit();

    TimeStamp whenCreated() const;
    TimeStamp lastModified() const;
    TimeStamp lastCommitted() const;
    TimeStamp childrenLastModified() const;

    void markAsModified();
    void markChildrenModified();

    bool subtreeModifiedButNotCommitted() const;
    bool anyChildModified() const;

   private:
    //! Use a custom provided node visitor to visit each node
    template <typename VISITOR_T>
    void traverse(VISITOR_T &&visitor, TraversalContext &ctx);

    struct
    {
      std::string name;
      NodeType type;
      std::string subType;
      std::string description;

      Any value;
      // vectors allows using length to determine if min/max is set
      std::vector<Any> minMax;
      bool readOnly;

      FlatMap<std::string, NodePtr> children;
      std::vector<Node *> parents;

      TimeStamp whenCreated;
      TimeStamp lastModified;
      TimeStamp childrenMTime;
      TimeStamp lastCommitted;
      TimeStamp lastVerified;
    } properties;

    void removeFromParentList(Node &node);

    friend NodePtr createNode(std::string, std::string, std::string, Any);

    friend struct CommitVisitor;
  };

  /////////////////////////////////////////////////////////////////////////////
  // Nodes with a strongly-typed value ////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  template <typename VALUE_T>
  struct Node_T : public Node
  {
    Node_T()                   = default;
    virtual ~Node_T() override = default;

    NodeType type() const override;

    const VALUE_T &value() const;

    template <typename OT>
    void operator=(OT &&val);

    operator VALUE_T();

   protected:
    void setOSPRayParam(std::string param, OSPObject obj) override;
  };

  // Pre-defined parameter nodes //////////////////////////////////////////////

  // OSPRay known parameter types //

  using StringNode  = Node_T<std::string>;
  using BoolNode    = Node_T<bool>;
  using FloatNode   = Node_T<float>;
  using Vec2fNode   = Node_T<vec2f>;
  using Vec3fNode   = Node_T<vec3f>;
  using Vec4fNode   = Node_T<vec4f>;
  using IntNode     = Node_T<int>;
  using Vec2iNode   = Node_T<vec2i>;
  using Vec3iNode   = Node_T<vec3i>;
  using Vec4iNode   = Node_T<vec4i>;
  using VoidPtrNode = Node_T<void *>;

  // Extra aliases //

  using Box3fNode   = Node_T<box3f>;
  using Box3iNode   = Node_T<box3i>;
  using Range1fNode = Node_T<range1f>;

  using RGBNode  = Node_T<rgb>;
  using RGBANode = Node_T<rgba>;

  using Transform = Node_T<affine3f>;

  /////////////////////////////////////////////////////////////////////////////
  // OSPRay Object Nodes //////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  template <typename HANDLE_T = cpp::ManagedObject<>,
            NodeType TYPE     = NodeType::GENERIC>
  struct OSPNode : public Node
  {
    OSPNode()                   = default;
    virtual ~OSPNode() override = default;

    NodeType type() const override;

    const HANDLE_T &handle() const;

    void setHandle(HANDLE_T handle);

    operator HANDLE_T();

   protected:
    virtual void preCommit() override;
    virtual void postCommit() override;

    void setOSPRayParam(std::string param, OSPObject obj) override;
  };

  /////////////////////////////////////////////////////////////////////////////
  // Main Node factory function ///////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  OSPSG_INTERFACE NodePtr createNode(std::string name,
                                     std::string subtype,
                                     std::string description,
                                     Any val);

  OSPSG_INTERFACE NodePtr createNode(std::string name);

  OSPSG_INTERFACE NodePtr createNode(std::string name, std::string subtype);

  OSPSG_INTERFACE NodePtr createNode(std::string name,
                                     std::string subtype,
                                     Any value);

  template <typename NODE_T, typename... Args>
  inline std::shared_ptr<NODE_T> createNodeAs(Args &&... args)
  {
    auto node = createNode(std::forward<Args>(args)...);
    return node->template nodeAs<NODE_T>();
  }

  /////////////////////////////////////////////////////////////////////////////
  // Node factory function registration ///////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

#define OSP_REGISTER_SG_NODE_NAME(InternalClassName, Name)                     \
  extern "C" OSPSG_DLLEXPORT ospray::sg::Node *ospray_create_sg_node__##Name() \
  {                                                                            \
    return new InternalClassName;                                              \
  }                                                                            \
  /* Extra declaration to avoid "extra ;" pedantic warnings */                 \
  ospray::sg::Node *ospray_create_sg_node__##Name()

#define OSP_REGISTER_SG_NODE(InternalClassName) \
  OSP_REGISTER_SG_NODE_NAME(InternalClassName, InternalClassName)

}  // namespace ospray::sg
}

#include "Node.inl"
