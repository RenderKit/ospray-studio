// Copyright 2009-2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "Visitor.h"
// stl
#include <map>
#include <unordered_map>
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
#include "ospray/ospray_cpp/ext/rkcommon.h"
#include "ospray/SDK/common/OSPCommon.h"
// ospray_sg
#include "version.h"
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

  // "filename" specialization allows differentiating purpose of string node.
  using filename = std::string;
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
    std::shared_ptr<T> nodeAs() const;

    template <typename T>
    std::shared_ptr<T> tryNodeAs();  // dynamic cast (slower, but can check)

    // Properties /////////////////////////////////////////////////////////////

    std::string name() const;
    virtual NodeType type() const;
    std::string subType() const;
    std::string description() const;

    // original node name(if present) as specified in a scene format file
    void setOrigName(const std::string &origName);
    std::string getOrigName();

    size_t uniqueID() const;

    // Node stored value (data) interface /////////////////////////////////////

    Any value();

    const Any value() const;

    template <typename T>
    T &valueAs();

    template <typename T>
    const T &valueAs() const;

    template <typename T>
    bool valueIsType() const;

    // updates modified time by default, special case does not
    template <typename T>
    void setValue(T val, bool markModified = true);

    void operator=(Any val);

    // Parent-child structural interface ///////////////////////////////////////

    using NodeLink = std::pair<std::string, NodePtr>;

    // Children //

    const FlatMap<std::string, NodePtr> &children() const;

    bool hasChildren() const;

    bool hasChild(const std::string &name) const;

    bool hasChildOfSubType(const std::string &subType) const;
    bool hasChildOfType(NodeType type) const;

    Node &child(const std::string &name);

    const std::vector<NodePtr> childrenOfSubType(const std::string &subType);
    const std::vector<NodePtr> childrenOfType(NodeType type);
    
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
    void killAllParents(); // DANGEROUS! Only used in sg loading
    void removeAllChildren();

    template <typename... Args>
    Node &createChild(Args &&... args);

    template <typename NODE_T, typename... Args>
    NODE_T &createChildAs(Args &&... args);

    template <typename... Args>
    void createChildData(std::string name, Args &&... args);

    void createChildData(std::string name, std::shared_ptr<Data> data);

    // Public method for self or any children modified
    inline bool isModified()
    {
      // True for self or any children, whereas anyChildModified doesn't
      // include self
      return subtreeModifiedButNotCommitted();
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
    void rmReadOnly();

    // Nodes that are used internally to the SG and invalid for OSPRay
    bool sgOnly() const;
    void setSGOnly();

    // Nodes that should not be shown in the UI 
    bool sgNoUI() const;
    void setSGNoUI();

   protected:
    virtual void preCommit();
    virtual void postCommit();

    TimeStamp whenCreated() const;
    TimeStamp lastModified() const;
    TimeStamp lastCommitted() const;
    TimeStamp childrenLastModified() const;

    void markAsModified();
    void updateChildrenModifiedTime();

    bool subtreeModifiedButNotCommitted() const;
    bool anyChildModified() const;

   private:
    //! Use a custom provided node visitor to visit each node
    template <typename VISITOR_T>
    void traverse(VISITOR_T &&visitor, TraversalContext &ctx);

    //! Use a custom provided node visitor to visit each node
    template <typename VISITOR_T>
    void traverseAnimation(VISITOR_T &&visitor, TraversalContext &ctx);

    struct
    {
      std::string name;
      NodeType type;
      std::string subType;
      std::string description;
      std::string origName;

      Any value;
      // vectors allows using length to determine if min/max is set
      std::vector<Any> minMax;

      // prevent user adjustment to this Node *via the UI*
      bool readOnly;
      // Nodes that are used internally to the SG and invalid for OSPRay
      bool sgOnly{false};
      // Nodes that should not be shown in the UI 
      bool sgNoUI{false};

      FlatMap<std::string, NodePtr> children;
      std::vector<Node *> parents;

      TimeStamp whenCreated;
      TimeStamp lastModified;
      TimeStamp childrenMTime;
      TimeStamp lastCommitted;
      TimeStamp lastVerified;
    } properties;

    void removeFromParentList(Node &node);

    friend NodePtr OSPSG_INTERFACE createNode(std::string, std::string, std::string, Any);

    friend struct CommitVisitor;
  };

  // SG Instance Picking //////////////////////////////////////////////////////

  typedef std::unordered_map<OSPInstance, unsigned int> OSPInstanceSGIdMap;
  typedef std::unordered_map<OSPGeometricModel, unsigned int>
      OSPGeomModelSGIdMap;

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
  using CharNode    = Node_T<char>;
  using UcharNode   = Node_T<unsigned char>;
  using IntNode     = Node_T<int>;
  using UIntNode    = Node_T<uint32_t>;
  using Vec2iNode   = Node_T<vec2i>;
  using Vec3iNode   = Node_T<vec3i>;
  using Vec4iNode   = Node_T<vec4i>;
  using VoidPtrNode = Node_T<void *>;
  using Box3fNode   = Node_T<box3f>;
  using Box3iNode   = Node_T<box3i>;
  using Range1fNode = Node_T<range1f>;
  using Affine3fNode = Node_T<affine3f>;
  using QuaternionfNode = Node_T<quaternionf>;
  using Linear2fNode = Node_T<linear2f>;


  // Extra aliases //

  using RGBNode  = Node_T<rgb>;
  using RGBANode = Node_T<rgba>;

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

    // updates modified time by default, special case does not
    void setHandle(HANDLE_T handle, bool markModified = true);

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

}  // namespace sg
} // namespace ospray

#include "Node.inl"
