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
#include <mutex>
// ospcommon
#include "ospcommon/utility/Any.h"
#include "ospcommon/utility/TimeStamp.h"
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

    // Base Node class definition /////////////////////////////////////////////

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

      void setName(const std::string &v);
      void setType(const std::string &v);
      void setDocumentation(const std::string &s);

      //! return a unique identifier for each created node
      size_t uniqueID() const;

      // Node stored value (data) interface ///////////////////////////////////

      //! get the value (copy) of the node, without template conversion
      Any value();

      //! returns the value of the node in the desired type
      template <typename T>
      T &valueAs();

      //! returns the value of the node in the desired type
      template <typename T>
      const T &valueAs() const;

      //! return if the value is the given type
      template <typename T>
      bool valueIsType() const;

      //! set the value of the node. Requires strict typecast
      template <typename T>
      void setValue(T val);

      //! set the value via the '=' operator
      template <typename T>
      void operator=(T &&val);

      // Update detection interface ///////////////////////////////////////////

      TimeStamp lastModified() const;
      TimeStamp lastCommitted() const;
      TimeStamp childrenLastModified() const;

      void markAsCommitted();
      virtual void markAsModified();
      virtual void setChildrenModified(TimeStamp t);

      //! Did this Node (or decendants) get modified and not (yet) committed?
      bool subtreeModifiedButNotCommitted() const;

      // Parent-child structual interface /////////////////////////////////////

      using NodeLink = std::pair<std::string, std::shared_ptr<Node>>;

      // Children //

      bool hasChild(const std::string &name) const;

      Node &child(const std::string &name) const;
      Node &operator[](const std::string &c) const;

      const std::map<std::string, std::shared_ptr<Node>> &children() const;

      bool hasChildren() const;
      size_t numChildren() const;

      void add(std::shared_ptr<Node> node);
      void add(std::shared_ptr<Node> node, const std::string &name);

      void remove(const std::string &name);

      //! just for convenience; add a typed 'setParam' function
      template <typename T>
      Node &createChildWithValue(const std::string &name,
                                 const std::string &type,
                                 const T &t);

      Node &createChild(std::string name,
                        std::string type          = "Node",
                        Any value                 = Any(),
                        std::string documentation = "");

      void setChild(const std::string &name, const std::shared_ptr<Node> &node);

      // Parent //

#if 0
      bool hasParent() const;

      Node &parent() const;
      void setParent(Node &p);
      void setParent(const std::shared_ptr<Node> &p);
#else
#warning implement parent query functions
#endif

      // Traversal interface //////////////////////////////////////////////////

      //! Use a custom provided node visitor to visit each node
      template <typename VISITOR_T, typename = is_valid_visitor_t<VISITOR_T>>
      void traverse(VISITOR_T &&visitor, TraversalContext &ctx);

      //! Helper overload to traverse with a default constructed TravesalContext
      template <typename VISITOR_T, typename = is_valid_visitor_t<VISITOR_T>>
      void traverse(VISITOR_T &&visitor);

     protected:
      struct
      {
        std::string name;
        std::string type;
        Any value;
        std::map<std::string, std::shared_ptr<Node>> children;
        std::map<std::string, std::shared_ptr<Node>> parents;
        TimeStamp whenCreated;
        TimeStamp lastModified;
        TimeStamp childrenMTime;
        TimeStamp lastCommitted;
        TimeStamp lastVerified;
        std::string documentation;
      } properties;
    };

    OSPSG_INTERFACE std::shared_ptr<Node> createNode(
        std::string name,
        std::string type          = "Node",
        Any var                   = Any(),
        std::string documentation = "");

    // Inlined Node definitions ///////////////////////////////////////////////

    template <typename T>
    inline std::shared_ptr<T> Node::nodeAs()
    {
      static_assert(std::is_base_of<Node, T>::value,
                    "Can only use nodeAs<T> to cast to an ospray::sg::Node"
                    " type! 'T' must be a child of ospray::sg::Node!");
      return std::static_pointer_cast<T>(shared_from_this());
    }

    template <typename T>
    inline std::shared_ptr<T> Node::tryNodeAs()
    {
      static_assert(std::is_base_of<Node, T>::value,
                    "Can only use tryNodeAs<T> to cast to an ospray::sg::Node"
                    " type! 'T' must be a child of ospray::sg::Node!");
      return std::dynamic_pointer_cast<T>(shared_from_this());
    }

    //! just for convenience; add a typed 'setParam' function
    template <typename T>
    inline Node &Node::createChildWithValue(const std::string &name,
                                            const std::string &type,
                                            const T &t)
    {
      if (hasChild(name)) {
        auto &c = child(name);
        c.setValue(t);
        return c;
      } else {
        auto node = createNode(name, type, t);
        add(node);
        return *node;
      }
    }

    template <typename T>
    inline void Node::setValue(T _val)
    {
      Any val(_val);
      bool modified = false;
      if (val != properties.value) {
        properties.value = val;
        modified         = true;
      }

      if (modified)
        markAsModified();
    }

    template <>
    inline void Node::setValue(Any val)
    {
      bool modified = false;
      if (val != properties.value) {
        properties.value = val;
        modified         = true;
      }

      if (modified)
        markAsModified();
    }

    template <typename T>
    inline T &Node::valueAs()
    {
      return properties.value.get<T>();
    }

    template <typename T>
    inline const T &Node::valueAs() const
    {
      return properties.value.get<T>();
    }

    template <typename T>
    inline bool Node::valueIsType() const
    {
      return properties.value.is<T>();
    }

    template <typename T>
    inline void Node::operator=(T &&v)
    {
      setValue(std::forward<T>(v));
    }

    // NOTE(jda) - Specialize valueAs() and operator=() so we don't have to
    //             convert to/from OSPObject manually, must trust the user to
    //             store/get the right type of OSPObject. This is because
    //             ospcommon::utility::Any<> cannot do implicit conversion...

#define DECLARE_VALUEAS_SPECIALIZATION(a)                \
  template <>                                            \
  inline a &Node::valueAs()                              \
  {                                                      \
    return (a &)properties.value.get<OSPObject>();       \
  }                                                      \
                                                         \
  template <>                                            \
  inline const a &Node::valueAs() const                  \
  {                                                      \
    return (const a &)properties.value.get<OSPObject>(); \
  }                                                      \
                                                         \
  template <>                                            \
  inline void Node::operator=(a &&v)                     \
  {                                                      \
    setValue((OSPObject)v);                              \
  }                                                      \
                                                         \
  template <>                                            \
  inline void Node::setValue(a val)                      \
  {                                                      \
    setValue((OSPObject)val);                            \
  }

    DECLARE_VALUEAS_SPECIALIZATION(OSPDevice)
    DECLARE_VALUEAS_SPECIALIZATION(OSPFrameBuffer)
    DECLARE_VALUEAS_SPECIALIZATION(OSPRenderer)
    DECLARE_VALUEAS_SPECIALIZATION(OSPCamera)
    DECLARE_VALUEAS_SPECIALIZATION(OSPWorld)
    DECLARE_VALUEAS_SPECIALIZATION(OSPData)
    DECLARE_VALUEAS_SPECIALIZATION(OSPGeometry)
    DECLARE_VALUEAS_SPECIALIZATION(OSPGeometryInstance)
    DECLARE_VALUEAS_SPECIALIZATION(OSPMaterial)
    DECLARE_VALUEAS_SPECIALIZATION(OSPLight)
    DECLARE_VALUEAS_SPECIALIZATION(OSPVolume)
    DECLARE_VALUEAS_SPECIALIZATION(OSPVolumeInstance)
    DECLARE_VALUEAS_SPECIALIZATION(OSPTransferFunction)
    DECLARE_VALUEAS_SPECIALIZATION(OSPTexture)
    DECLARE_VALUEAS_SPECIALIZATION(OSPPixelOp)
    DECLARE_VALUEAS_SPECIALIZATION(OSPFuture)

#undef DECLARE_VALUEAS_SPECIALIZATION

    template <typename VISITOR_T, typename>
    inline void Node::traverse(VISITOR_T &&visitor, TraversalContext &ctx)
    {
      static_assert(is_valid_visitor<VISITOR_T>::value,
                    "VISITOR_T must be a child class of sg::Visitor or"
                    " implement 'bool visit(Node &node, TraversalContext &ctx)'"
                    "!");

      bool traverseChildren = visitor(*this, ctx);

      ctx.level++;

      if (traverseChildren) {
        for (auto &child : properties.children)
          child.second->traverse(visitor, ctx);
      }

      ctx.level--;

      visitor.postChildren(*this, ctx);
    }

    template <typename VISITOR_T, typename>
    inline void Node::traverse(VISITOR_T &&visitor)
    {
      TraversalContext ctx;
      traverse(std::forward<VISITOR_T>(visitor), ctx);
    }

    /*! \brief registers a internal ospray::<ClassName> renderer under
      the externally accessible name "external_name"

      \internal This currently works by defining a extern "C" function
      with a given predefined name that creates a new instance of this
      renderer. By having this symbol in the shared lib ospray can
      lateron always get a handle to this fct and create an instance
      of this renderer.
    */
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
