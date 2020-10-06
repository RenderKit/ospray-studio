// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "Data.h"
#include <json.hpp>

namespace ospray {
  namespace sg {

  /////////////////////////////////////////////////////////////////////////////
  // Inlined Node definitions /////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  template <typename T>
  inline std::shared_ptr<T> Node::nodeAs()
  {
    static_assert(std::is_base_of<Node, T>::value,
                  "Can only use nodeAs<T> to cast to an ospray::sg::Node"
                  " type! 'T' must be a child of ospray::sg::Node!");
    return std::static_pointer_cast<T>(shared_from_this());
  }

  template <typename T>
  inline std::shared_ptr<T> Node::nodeAs() const
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

  template <typename NODE_T>
  NODE_T &Node::childAs(const std::string &name)
  {
    return *child(name).nodeAs<NODE_T>();
  }

  template <typename NODE_T>
  std::shared_ptr<NODE_T> Node::childNodeAs(const std::string &name)
  {
    return child(name).nodeAs<NODE_T>();
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

  inline void Node::operator=(Any v)
  {
    setValue(v);
  }

  template <typename... Args>
  inline Node &Node::createChild(Args &&... args)
  {
    auto child = createNode(std::forward<Args>(args)...);
    add(child);
    return *child;
  }

  template <typename NODE_T, typename... Args>
  inline NODE_T &Node::createChildAs(Args &&... args)
  {
    auto &child = createChild(std::forward<Args>(args)...);
    return *child.template nodeAs<NODE_T>();
  }

  template <typename... Args>
  inline void Node::createChildData(std::string name, Args &&... args)
  {
    auto data = std::make_shared<Data>(std::forward<Args>(args)...);
    if (data) {
      data->properties.name = name;
      data->properties.subType = "Data";
      add(data);
    }
  }

  template <typename VISITOR_T>
  inline void Node::traverse(VISITOR_T &&visitor)
  {
    TraversalContext ctx;
    traverse(std::forward<VISITOR_T>(visitor), ctx);
  }

  template <typename VISITOR_T, typename... Args>
  inline void Node::traverse(Args &&... args)
  {
    traverse(VISITOR_T(std::forward<Args>(args)...));
  }

  template <typename VISITOR_T>
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

  template <typename VISITOR_T>
  inline void Node::traverseAnimation(TraversalContext &ctx, VISITOR_T &&visitor)
  {
    traverseAnimation(std::forward<VISITOR_T>(visitor), ctx);
  }


  template <typename VISITOR_T, typename... Args>
  inline void Node::traverseAnimation(NodePtr animationWorld, Args &&... args)
  {
    TraversalContext ctx;

    ctx.animationWorld = animationWorld;

    traverseAnimation(ctx, VISITOR_T(std::forward<Args>(args)...));

  }

  template <typename VISITOR_T>
  inline void Node::traverseAnimation(VISITOR_T &&visitor, TraversalContext &ctx)
  {
    static_assert(is_valid_visitor<VISITOR_T>::value,
                  "VISITOR_T must be a child class of sg::Visitor or"
                  " implement 'bool visit(Node &node, TraversalContext &ctx)'"
                  "!");

    bool traverseChildren = visitor(*this, ctx);

    ctx.level++;

    if (traverseChildren) {
      for (auto &child : properties.children)
        child.second->traverseAnimation(visitor, ctx);
    }

    ctx.level--;

    visitor.postChildren(*this, ctx);
  }

  template <typename T>
  inline const T &Node::minAs() const
  {
    return properties.minMax[0].get<T>();
  }

  template <typename T>
  inline const T &Node::maxAs() const
  {
    return properties.minMax[1].get<T>();
  }

  inline bool Node::subtreeModifiedButNotCommitted() const
  {
    return (lastModified() > lastCommitted()) ||
           (childrenLastModified() > lastCommitted());
  }

  inline bool Node::anyChildModified() const
  {
    for (auto &child : properties.children) {
      if (child.second->subtreeModifiedButNotCommitted())
        return true;
    }

    return false;
  }

  /////////////////////////////////////////////////////////////////////////////
  // Inlined Node_T<> definitions /////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  template <typename VALUE_T>
  inline NodeType Node_T<VALUE_T>::type() const
  {
    return NodeType::PARAMETER;
  }

  template <>
  inline NodeType Transform::type() const
  {
    return NodeType::TRANSFORM;
  }

  template <typename VALUE_T>
  inline const VALUE_T &Node_T<VALUE_T>::value() const
  {
    return Node::valueAs<VALUE_T>();
  }

  template <typename VALUE_T>
  template <typename OT>
  inline void Node_T<VALUE_T>::operator=(OT &&val)
  {
    Node::operator=(static_cast<VALUE_T>(val));
  }

  template <typename VALUE_T>
  inline Node_T<VALUE_T>::operator VALUE_T()
  {
    return value();
  }

  template <typename VALUE_T>
  inline void Node_T<VALUE_T>::setOSPRayParam(std::string param, OSPObject obj)
  {
    static_assert(OSPTypeFor<VALUE_T>::value != OSP_UNKNOWN,
      "setOSPRayParam: unknown node type");
    ospSetParam(obj, param.c_str(), OSPTypeFor<VALUE_T>::value, &value());
  }

  template <>
  inline void Node_T<std::string>::setOSPRayParam(std::string param,
                                                  OSPObject obj)
  {
    auto *c_str = value().c_str();
    ospSetParam(obj, param.c_str(), OSP_STRING, c_str);
  }

  /////////////////////////////////////////////////////////////////////////////
  // Inlined OSPNode definitions //////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  template <typename HANDLE_T, NodeType TYPE>
  inline NodeType OSPNode<HANDLE_T, TYPE>::type() const
  {
    return TYPE;
  }

  template <typename HANDLE_T, NodeType TYPE>
  inline const HANDLE_T &OSPNode<HANDLE_T, TYPE>::handle() const
  {
    return Node::valueAs<HANDLE_T>();
  }

  template <typename HANDLE_T, NodeType TYPE>
  inline void OSPNode<HANDLE_T, TYPE>::setHandle(HANDLE_T handle)
  {
    setValue(handle);
  }

  template <typename HANDLE_T, NodeType TYPE>
  inline OSPNode<HANDLE_T, TYPE>::operator HANDLE_T()
  {
    return handle();
  }

  template <typename HANDLE_T, NodeType TYPE>
  inline void OSPNode<HANDLE_T, TYPE>::preCommit()
  {
    for (auto &c : children()) {
      if (c.second->type() == NodeType::PARAMETER)
        if (!c.second->sgOnly())
          c.second->setOSPRayParam(c.first, handle().handle());
    }
  }

  template <typename HANDLE_T, NodeType TYPE>
  inline void OSPNode<HANDLE_T, TYPE>::postCommit()
  {
    handle().commit();
  }

  template <typename HANDLE_T, NodeType TYPE>
  inline void OSPNode<HANDLE_T, TYPE>::setOSPRayParam(std::string param,
                                                      OSPObject obj)
  {
    static_assert(OSPTypeFor<HANDLE_T>::value != OSP_UNKNOWN,
      "setOSPRayParam: unknown node type");
    auto h = handle().handle();
    ospSetParam(obj, param.c_str(), OSPTypeFor<HANDLE_T>::value, &h);
  }

  }  // namespace sg
} // namespace ospray
