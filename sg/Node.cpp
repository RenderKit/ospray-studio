// Copyright 2009-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Node.h"
#include "visitors/Commit.h"
#include "visitors/GetBounds.h"
#include "visitors/RenderScene.h"
// rkcommon
#include "rkcommon/os/library.h"
#include "rkcommon/utility/StringManip.h"

namespace ospray {
  namespace sg {

  /////////////////////////////////////////////////////////////////////////////

  Node::Node()
  {
    // NOTE(jda) - can't do default member initializers due to MSVC...
    properties.name        = "NULL";
    properties.type        = NodeType::GENERIC;
    properties.subType     = "Node";
    properties.description = "<no description>";
    properties.readOnly    = false;
  }

  /////////////////////////////////////////////////////////////////////////////
  // Properties ///////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  std::string Node::name() const
  {
    return properties.name;
  }

  NodeType Node::type() const
  {
    return properties.type;
  }

  std::string Node::subType() const
  {
    return properties.subType;
  }

  std::string Node::description() const
  {
    return properties.description;
  }

  size_t Node::uniqueID() const
  {
    return properties.whenCreated;
  }

  /////////////////////////////////////////////////////////////////////////////
  // Node stored value (data) interface ///////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  Any Node::value()
  {
    return properties.value;
  }

  const Any Node::value() const
  {
    return properties.value;
  }

  /////////////////////////////////////////////////////////////////////////////
  // Parent-child structual interface /////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  const FlatMap<std::string, NodePtr> &Node::children() const
  {
    return properties.children;
  }

 bool Node::hasChild(const std::string &name) const
  {
    auto &c = properties.children;
    if (c.contains(name))
      return true;

    auto itr = std::find_if(c.cbegin(), c.cend(), [&](const NodeLink &n) {
      return n.first == name;
    });

    return itr != properties.children.end();
  }

  bool Node::hasChildOfType(const std::string &subType) const
  {
    auto &c = properties.children;

    auto itr = std::find_if(c.cbegin(), c.cend(), [&](const NodeLink &n) {
      return n.second->subType() == subType;
    });

    return itr != properties.children.end();
  }

  Node &Node::child(const std::string &name)
  {
    auto &c = properties.children;
    if (c.contains(name))
      return *c[name];

    auto itr = std::find_if(c.begin(), c.end(), [&](const NodeLink &n) {
      return n.first == name;
    });

    if (itr == properties.children.cend()) {
      throw std::runtime_error(
          "in " + subType() + " node '" + this->name() + "'" +
          ": could not find sg child node with name '" + name + "'");
    } else {
      return *itr->second;
    }
  }

  const std::vector<NodePtr> Node::childrenOfType(const std::string &subType)
  {
    auto &props = properties.children;
    std::vector<NodePtr> childrenOfType;

    for (auto &p : props) {
      if (p.second->subType() == subType)
        childrenOfType.push_back(p.second);
    }
    return childrenOfType;
  }

  Node &Node::operator[](const std::string &c)
  {
    return child(c);
  }

  bool Node::hasChildren() const
  {
    return !properties.children.empty();
  }

  const std::vector<Node *> &Node::parents() const
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
    if (hasChild(name)) {
      if (properties.children[name] == node)
        return;
      properties.children[name]->removeFromParentList(*this);
    }
    properties.children[name] = node;
    node->properties.parents.push_back(this);
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

  /////////////////////////////////////////////////////////////////////////////
  // Traveral Interface ///////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  void Node::commit()
  {
    traverse<CommitVisitor>();
  }

  void Node::render()
  {
    commit();
    traverse<RenderScene>();
  }

  void Node::render(GeomIdMap &geomIdMap, InstanceIdMap &instanceIdMap)
  {
    commit();
    traverse<RenderScene>(geomIdMap, instanceIdMap);
  }

  box3f Node::bounds()
  {
    GetBounds visitor;
    traverse(visitor);
    return visitor.bounds;
  }

  /////////////////////////////////////////////////////////////////////////////
  // UI Generation ////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  Any Node::min() const
  {
    return properties.minMax[0];
  }

  Any Node::max() const
  {
    return properties.minMax[1];
  }

  void Node::setMinMax(const Any &minVal, const Any &maxVal)
  {
    properties.minMax.resize(2);
    properties.minMax[0] = minVal;
    properties.minMax[1] = maxVal;
  }

  bool Node::hasMinMax() const
  {
    return (properties.minMax.size() > 1);
  }

  bool Node::readOnly() const
  {
    return properties.readOnly;
  }

  void Node::setReadOnly()
  {
    properties.readOnly = true;
  }

  bool Node::sgOnly() const
  {
    return properties.sgOnly;
  }

  void Node::setSGOnly()
  {
    properties.sgOnly = true;
  }

  bool Node::sgNoUI() const
  {
    return properties.sgNoUI;
  }

  void Node::setSGNoUI()
  {
    properties.sgNoUI = true;
  }

  /////////////////////////////////////////////////////////////////////////////
  // Private Members //////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  void Node::preCommit() {}

  void Node::postCommit() {}

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

  void Node::removeFromParentList(Node &node)
  {
    node.markAsModified(); // Removal requires notifying parents
    auto &p          = properties.parents;
    auto remove_node = [&](auto np) { return np == &node; };
    p.erase(std::remove_if(p.begin(), p.end(), remove_node), p.end());
  }

  void Node::markAsModified()
  {
    // Mark all parents, up to root, as modified
    properties.lastModified.renew();
    for (auto &p : properties.parents)
      p->updateChildrenModifiedTime();
  }

  void Node::updateChildrenModifiedTime()
  {
    // Notify all parent of latest child modified time
    properties.childrenMTime.renew();
    for (auto &p : properties.parents)
      p->updateChildrenModifiedTime();
  }

  void Node::setOSPRayParam(std::string, OSPObject) {}

  /////////////////////////////////////////////////////////////////////////////
  // Global Stuff /////////////////////////////////////////////////////////////
  /////////////////////////////////////////////////////////////////////////////

  using CreatorFct = Node *(*)();

  static std::map<std::string, CreatorFct> nodeRegistry;

  std::shared_ptr<Node> createNode(std::string name,
                                   std::string subtype,
                                   std::string description,
                                   Any value)
  {
    // Verify that 'ospray_sg' is properly loaded //

    static bool libraryLoaded = false;
    if (!libraryLoaded) {
      loadLibrary("ospray_sg");
      libraryLoaded = true;
    }

    // Look for the factory function to create the node //

    auto it = nodeRegistry.find(subtype);

    CreatorFct creator = nullptr;

    if (it == nodeRegistry.end()) {
      std::string creatorName = "ospray_create_sg_node__" + subtype;

      creator = (CreatorFct)getSymbol(creatorName);
      if (!creator)
        throw std::runtime_error("unknown node type '" + subtype + "'");

      nodeRegistry[subtype] = creator;
    } else {
      creator = it->second;
    }

    // Create the node and return //

    std::shared_ptr<sg::Node> newNode(creator());
    newNode->properties.name        = name;
    newNode->properties.subType     = subtype;
    newNode->properties.type        = newNode->type();
    newNode->properties.description = description;

    if (value.valid())
      newNode->setValue(value);

    return newNode;
  }

  NodePtr createNode(std::string name)
  {
    return createNode(name, "Node");
  }

  NodePtr createNode(std::string name, std::string subtype)
  {
    return createNode(name, subtype, "<no description>", Any());
  }

  NodePtr createNode(std::string name, std::string subtype, Any value)
  {
    return createNode(name, subtype, "<no description>", value);
  }

  OSP_REGISTER_SG_NODE(Node);

  // Node_T<> type names

  OSP_REGISTER_SG_NODE_NAME(StringNode, string);
  OSP_REGISTER_SG_NODE_NAME(BoolNode, bool);
  OSP_REGISTER_SG_NODE_NAME(FloatNode, float);
  OSP_REGISTER_SG_NODE_NAME(Vec2fNode, vec2f);
  OSP_REGISTER_SG_NODE_NAME(Vec3fNode, vec3f);
  OSP_REGISTER_SG_NODE_NAME(Vec4fNode, vec4f);
  OSP_REGISTER_SG_NODE_NAME(CharNode, char);
  OSP_REGISTER_SG_NODE_NAME(UcharNode, uchar);
  OSP_REGISTER_SG_NODE_NAME(IntNode, int);
  OSP_REGISTER_SG_NODE_NAME(UIntNode, uint32_t);
  OSP_REGISTER_SG_NODE_NAME(Vec2iNode, vec2i);
  OSP_REGISTER_SG_NODE_NAME(Vec3iNode, vec3i);
  OSP_REGISTER_SG_NODE_NAME(Vec4iNode, vec4i);
  OSP_REGISTER_SG_NODE_NAME(VoidPtrNode, void_ptr);
  OSP_REGISTER_SG_NODE_NAME(Box3fNode, box3f);
  OSP_REGISTER_SG_NODE_NAME(Box3iNode, box3i);
  OSP_REGISTER_SG_NODE_NAME(Range1fNode, range1f);
  OSP_REGISTER_SG_NODE_NAME(Affine3fNode, affine3f);
  OSP_REGISTER_SG_NODE_NAME(QuaternionfNode, quaternionf);

  OSP_REGISTER_SG_NODE_NAME(RGBNode, rgb);
  OSP_REGISTER_SG_NODE_NAME(RGBANode, rgba);

  }  // namespace sg
} // namespace ospray
