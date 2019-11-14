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

namespace ospray {
  namespace sg {

    ///////////////////////////////////////////////////////////////////////////
    // Inlined Node definitions ///////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

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

    template <typename NODE_T>
    NODE_T &Node::childAs(const std::string &name)
    {
      return *child(name).nodeAs<NODE_T>();
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

    inline bool Node::subtreeModifiedButNotCommitted() const
    {
      return (lastModified() > lastCommitted()) ||
             (childrenLastModified() > lastCommitted());
    }

    ///////////////////////////////////////////////////////////////////////////
    // Inlined Node_T<> definitions ///////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    template <typename VALUE_T>
    inline NodeType Node_T<VALUE_T>::type() const
    {
      return NodeType::PARAMETER;
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
    inline void Node_T<VALUE_T>::setOSPRayParam(std::string param,
                                                OSPObject handle)
    {
      ospSetParam(handle, param.c_str(), OSPTypeFor<VALUE_T>::value, &value());
    }

    ///////////////////////////////////////////////////////////////////////////
    // Inlined OSPNode definitions ////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    template <typename HANDLE_T>
    inline OSPNode<HANDLE_T>::OSPNode()
    {
      setHandle(nullptr);
    }

    template <typename HANDLE_T>
    inline OSPNode<HANDLE_T>::~OSPNode()
    {
      auto h = handle();
      if (h)
        ospRelease(handle());
    }

    template <typename HANDLE_T>
    inline NodeType OSPNode<HANDLE_T>::type() const
    {
      return NodeType::PARAMETER;
    }

    template <typename HANDLE_T>
    inline const HANDLE_T &OSPNode<HANDLE_T>::handle() const
    {
      return Node::valueAs<HANDLE_T>();
    }

    template <typename HANDLE_T>
    inline void OSPNode<HANDLE_T>::setHandle(HANDLE_T handle)
    {
      setValue(handle);
    }

    template <typename HANDLE_T>
    inline OSPNode<HANDLE_T>::operator HANDLE_T()
    {
      return handle();
    }

    template <typename HANDLE_T>
    inline void OSPNode<HANDLE_T>::preCommit()
    {
      for (auto &c : children())
        c.second->setOSPRayParam(c.first, handle());
    }

    template <typename HANDLE_T>
    inline void OSPNode<HANDLE_T>::postCommit()
    {
      ospCommit(handle());
    }

    template <typename HANDLE_T>
    inline void OSPNode<HANDLE_T>::setOSPRayParam(std::string param,
                                                  OSPObject obj)
    {
      ospSetObject(obj, param.c_str(), handle());
    }

  }  // namespace sg
}  // namespace ospray
