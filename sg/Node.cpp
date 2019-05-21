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

#include "Node.h"
// ospcommon
#include "ospcommon/library.h"
#include "ospcommon/utility/StringManip.h"

namespace ospray {
  namespace sg {

    Node::Node()
    {
      // NOTE(jda) - can't do default member initializers due to MSVC...
      properties.name = "NULL";
      properties.type = "Node";
    }

    Node::~Node()
    {
      if (valueIsType<OSPObject>())
        ospRelease(valueAs<OSPObject>());
    }

    std::string Node::toString() const
    {
      return "ospray::sg::Node";
    }

    ///////////////////////////////////////////////////////////////////////////
    // Properties /////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    std::string Node::name() const
    {
      return properties.name;
    }

    std::string Node::type() const
    {
      return properties.type;
    }

    std::string Node::documentation() const
    {
      return properties.documentation;
    }

    size_t Node::uniqueID() const
    {
      return properties.whenCreated;
    }

    ///////////////////////////////////////////////////////////////////////////
    // Node stored value (data) interface /////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    Any Node::value()
    {
      return properties.value;
    }

    ///////////////////////////////////////////////////////////////////////////
    // Update detection interface /////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    TimeStamp Node::whenCreated() const
    {
      return properties.whenCreated;
    }

    TimeStamp Node::lastModified() const
    {
      return properties.lastModified;
    }

    TimeStamp Node::lastCommitted() const
    {
      return properties.lastCommitted;
    }

    TimeStamp Node::childrenLastModified() const
    {
      return properties.childrenMTime;
    }

    ///////////////////////////////////////////////////////////////////////////
    // Parent-child structual interface ///////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    const std::map<std::string, std::shared_ptr<Node>> &Node::children() const
    {
      return properties.children;
    }

    bool Node::hasChild(const std::string &name) const
    {
      auto itr = properties.children.find(name);
      if (itr != properties.children.end())
        return true;

      std::string name_lower = utility::lowerCase(name);

      auto &c = properties.children;
      itr     = std::find_if(c.begin(), c.end(), [&](const NodeLink &n) {
        return utility::lowerCase(n.first) == name_lower;
      });

      return itr != properties.children.end();
    }

    Node &Node::child(const std::string &name) const
    {
      auto itr = properties.children.find(name);
      if (itr != properties.children.end())
        return *itr->second;

      std::string name_lower = utility::lowerCase(name);

      auto &c = properties.children;
      itr     = std::find_if(c.begin(), c.end(), [&](const NodeLink &n) {
        return utility::lowerCase(n.first) == name_lower;
      });

      if (itr == properties.children.end()) {
        throw std::runtime_error("in node " + toString() +
                                 " : could not find sg child node with name '" +
                                 name + "'");
      } else {
        return *itr->second;
      }
    }

    Node &Node::operator[](const std::string &c) const
    {
      return child(c);
    }

    bool Node::hasChildren() const
    {
      return !properties.children.empty();
    }

    const std::vector<NodePtr> &Node::parents() const
    {
      return properties.parents;
    }

    bool Node::hasParents() const
    {
      return !properties.parents.empty();
    }

    void Node::add(Node &node)
    {
      add(node.shared_from_this());
    }

    void Node::add(Node &node, const std::string &name)
    {
      add(node.shared_from_this(), name);
    }

    void Node::add(NodePtr node)
    {
      add(node, node->name());
    }

    void Node::add(NodePtr node, const std::string &name)
    {
      properties.children[name] = node;
      node->properties.parents.push_back(shared_from_this());
      markAsModified();
    }

    void Node::remove(Node &node)
    {
      for (auto &child : properties.children) {
        if (child.second.get() == &node) {
          child.second->removeFromParentList(*this);
          properties.children.erase(child.first);
          return;
        }
      }

      markAsModified();
    }

    void Node::remove(NodePtr node)
    {
      remove(*node);
    }

    void Node::remove(const std::string &name)
    {
      for (auto &child : properties.children) {
        if (child.first == name) {
          child.second->removeFromParentList(*this);
          properties.children.erase(child.first);
          return;
        }
      }

      markAsModified();
    }

    void Node::removeAllParents()
    {
      for (auto &p : properties.parents)
        p->remove(*this);
    }

    void Node::removeAllChildren()
    {
      for (auto &c : properties.children)
        remove(c.first);
    }

    Node &Node::createChild(std::string name,
                            std::string type,
                            Any value,
                            std::string documentation)
    {
      auto child = createNode(name, type, value, documentation);
      add(child);
      return *child;
    }

    ///////////////////////////////////////////////////////////////////////////
    // Private Members ////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    void Node::removeFromParentList(Node &node)
    {
      auto &p          = properties.parents;
      auto remove_node = [&](NodePtr np) { return np.get() == &node; };
      p.erase(std::remove_if(p.begin(), p.end(), remove_node), p.end());
    }

    void Node::markAsModified()
    {
      properties.lastModified.renew();
      for (auto &p : properties.parents)
        p->setChildrenModified(properties.lastModified);
    }

    void Node::setChildrenModified(TimeStamp t)
    {
      if (t > properties.childrenMTime) {
        properties.childrenMTime = t;
        for (auto &p : properties.parents)
          p->setChildrenModified(properties.childrenMTime);
      }
    }

    void Node::setName(const std::string &v)
    {
      properties.name = v;
    }

    void Node::setType(const std::string &v)
    {
      properties.type = v;
    }

    void Node::setDocumentation(const std::string &s)
    {
      properties.documentation = s;
    }

    ///////////////////////////////////////////////////////////////////////////
    // Global Stuff ///////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////

    using CreatorFct = Node *(*)();

    static std::map<std::string, CreatorFct> nodeRegistry;

    std::shared_ptr<Node> createNode(std::string name,
                                     std::string type,
                                     Any value,
                                     std::string documentation)
    {
      // Verify that 'ospray_sg' is properly loaded //

      static bool libraryLoaded = false;
      if (!libraryLoaded) {
        loadLibrary("ospray_sg");
        libraryLoaded = true;
      }

      // Look for the factory function to create the node //

      auto it = nodeRegistry.find(type);

      CreatorFct creator = nullptr;

      if (it == nodeRegistry.end()) {
        std::string creatorName = "ospray_create_sg_node__" + type;

        creator = (CreatorFct)getSymbol(creatorName);
        if (!creator)
          throw std::runtime_error("unknown node type '" + type + "'");

        nodeRegistry[type] = creator;
      } else {
        creator = it->second;
      }

      // Create the node and return //

      std::shared_ptr<sg::Node> newNode(creator());
      newNode->setName(name);
      newNode->setType(type);
      newNode->setDocumentation(documentation);

      if (value.valid())
        newNode->setValue(value);

      return newNode;
    }

    OSP_REGISTER_SG_NODE(Node);

    OSP_REGISTER_SG_NODE_NAME(StringNode, string);
    OSP_REGISTER_SG_NODE_NAME(BoolNode, bool);
    OSP_REGISTER_SG_NODE_NAME(FloatNode, float);
    OSP_REGISTER_SG_NODE_NAME(Vec2fNode, vec2f);
    OSP_REGISTER_SG_NODE_NAME(Vec3fNode, vec3f);
    OSP_REGISTER_SG_NODE_NAME(Vec4fNode, vec4f);
    OSP_REGISTER_SG_NODE_NAME(IntNode, int);
    OSP_REGISTER_SG_NODE_NAME(Vec2iNode, vec2i);
    OSP_REGISTER_SG_NODE_NAME(Vec3iNode, vec3i);
    OSP_REGISTER_SG_NODE_NAME(VoidPtrNode, void_ptr);

  }  // namespace sg
}  // namespace ospray
