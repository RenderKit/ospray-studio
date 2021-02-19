// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// pybind11 headers
#include <pybind11/numpy.h>
#include <pybind11/operators.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

// SG classes
#include <sg/Frame.h>
#include <sg/Node.h>
#include <sg/camera/Camera.h>
#include <sg/fb/FrameBuffer.h>
#include <sg/renderer/MaterialRegistry.h>
#include <sg/renderer/Renderer.h>
#include <sg/scene/World.h>
#include <sg/scene/geometry/Geometry.h>
#include <sg/scene/lights/LightsManager.h>
#include <sg/Data.h>

namespace py = pybind11;
using namespace ospray::sg;

static std::vector<std::string> init(const std::vector<std::string> &args)
{
  int argc = args.size();
  const char **argv = new const char *[argc];

  for (int i = 0; i < argc; i++)
    argv[i] = args[i].c_str();

  OSPError initError = ospInit(&argc, argv);

  if (initError != OSP_NO_ERROR) {
    delete[] argv;
    std::cerr << "OSPRay not initialized correctly!" << std::endl;
  }

  std::vector<std::string> newargs;

  for (int i = 0; i < argc; i++)
    newargs.push_back(std::string(argv[i]));

  delete[] argv;

  return newargs;
}

// OSPNode typedefs //////////////////////////////////////////////////////

typedef OSPNode<ospray::cpp::Camera, NodeType::CAMERA> OSPNodeCamera;
typedef OSPNode<ospray::cpp::Future, NodeType::FRAME> OSPNodeFrame;
typedef OSPNode<ospray::cpp::Renderer, NodeType::RENDERER> OSPNodeRenderer;
typedef OSPNode<ospray::cpp::FrameBuffer, NodeType::FRAME_BUFFER>
    OSPNodeFrameBuffer;
typedef OSPNode<ospray::cpp::World, NodeType::WORLD> OSPNodeWorld;
typedef OSPNode<ospray::cpp::Geometry, NodeType::GEOMETRY> OSPNodeGeometry;
typedef OSPNode<ospray::cpp::Light, NodeType::LIGHTS> OSPNodeLightsManager;
typedef OSPNode<ospray::cpp::CopiedData, NodeType::PARAMETER> OSPNodeData;

// instantiation for NodeTypes ///////////////////////////////////////////

template <typename T>
void pysg_nodeType(py::module &sg, const char *name)
{
  py::class_<Node_T<T>, Node, std::shared_ptr<Node_T<T>>>(sg, name).def(
      py::init<>());
}

template <typename T>
void pysg_ospNodeType(py::module &sg, const char *name)
{
  py::class_<T, Node, std::shared_ptr<T>>(sg, name).def(py::init<>());
}

template <typename T>
void pysg_vec2Type(py::module &sg, const char *name)
{
  py::class_<vec_t<T, 2>>(sg, name).def(py::init<T>()).def(py::init<T, T>());
}

template <typename T>
void pysg_vec3Type(py::module &sg, const char *name)
{
  py::class_<vec_t<T, 3>>(sg, name).def(py::init<T>()).def(py::init<T, T, T>());
}

template <typename T>
void pysg_vec4Type(py::module &sg, const char *name)
{
  py::class_<vec_t<T, 4>>(sg, name)
      .def(py::init<T>())
      .def(py::init<T, T, T, T>());
}

// Main SG python Module ///////////////////////////////////////////////////

PYBIND11_MODULE(pysg, sg)
{
  // OSPRay initialization 

  sg.def("init", &init);

  // Main Node factory function ///////////////////////////////////////////

  sg.def(
      "createNode", py::overload_cast<std::string, std::string>(&createNode));
  sg.def("createNode",
      py::overload_cast<std::string, std::string, rkcommon::utility::Any>(
          &createNode));

  // commonly used vector types ////////////////////////////////////////////

  pysg_vec2Type<float>(sg, "vec2f");
  pysg_vec3Type<float>(sg, "vec3f");
  pysg_vec4Type<float>(sg, "vec4f");
  pysg_vec2Type<int>(sg, "vec2i");
  pysg_vec3Type<int>(sg, "vec3i");
  pysg_vec4Type<int>(sg, "vec4i");
  pysg_vec2Type<uint32_t>(sg, "vec2ui");
  pysg_vec3Type<uint32_t>(sg, "vec3ui");
  pysg_vec4Type<uint32_t>(sg, "vec4ui");

  // rkcommon::utility::Any class with overloaded constructor ////////////////

  py::class_<Any>(sg, "Any")
      .def(py::init<std::string>())
      .def(py::init<bool>())
      .def(py::init<char>())
      .def(py::init<int>())
      .def(py::init<unsigned char>())
      .def(py::init<uint32_t>())
      .def(py::init<float>())
      .def(py::init<vec2f>())
      .def(py::init<vec3f>())
      .def(py::init<vec4f>());

  // Generic Node class definition ////////////////////////////////////////////

  py::class_<Node, std::shared_ptr<Node>>(sg, "Node")
      .def(py::init<>())
      .def("createChild",
          static_cast<Node &(
              Node::*)(const std::string &, const std::string &, Any &)>(
              &Node::createChild),
          py::return_value_policy::reference)
      .def("createChild",
          static_cast<Node &(Node::*)(const std::string &,
              const std::string &)>(&Node::createChild),
          py::return_value_policy::reference)
      .def("add", py::overload_cast<NodePtr>(&Node::add))
      .def("commit", &Node::commit)
      .def("render", py::overload_cast<>(&Node::render))
      .def("child", &Node::child, py::return_value_policy::reference)
      .def("createChildData",
          static_cast<void (Node::*)(std::string, std::shared_ptr<Data>)>(
              &Node::createChildData))
      .def("createChildData",
          static_cast<void (Node::*)(std::string, vec3f &)>(
              &Node::createChildData));

  // Nodes with strongly-types values ////////////////////////////////////////

  pysg_nodeType<std::string>(sg, "StringNode");
  pysg_nodeType<bool>(sg, "BoolNode");
  pysg_nodeType<float>(sg, "FloatNode");
  pysg_nodeType<char>(sg, "CharNode");
  pysg_nodeType<unsigned char>(sg, "UcharNode");
  pysg_nodeType<int>(sg, "IntNode");
  pysg_nodeType<uint32_t>(sg, "UIntNode");
  pysg_nodeType<vec2f>(sg, "Vec2fNode");
  pysg_nodeType<vec3f>(sg, "Vec3fNode");
  pysg_nodeType<vec4f>(sg, "Vec4fNode");
  pysg_nodeType<void *>(sg, "VoidPtrNode");
  pysg_nodeType<box3f>(sg, "Box3fNode");
  pysg_nodeType<box3i>(sg, "Box3iNode");
  pysg_nodeType<range1f>(sg, "Range1fNode");
  pysg_nodeType<affine3f>(sg, "Affine3fNode");
  pysg_nodeType<quaternionf>(sg, "QuaternionfNode");

  // pysg_nodeType<rgb>(sg, "RGBNode");
  // pysg_nodeType<rgba>(sg, "RGBANode");

  // OSPRAY Object Nodes ////////////////////////////////////////////////////

  pysg_ospNodeType<OSPNodeCamera>(sg, "OSPCamera");
  pysg_ospNodeType<OSPNodeFrame>(sg, "OSPFrame");
  pysg_ospNodeType<OSPNodeRenderer>(sg, "OSPRenderer");
  pysg_ospNodeType<OSPNodeFrameBuffer>(sg, "OSPFrameBuffer");
  pysg_ospNodeType<OSPNodeWorld>(sg, "OSPWorld");
  pysg_ospNodeType<OSPNodeGeometry>(sg, "OSPGeometry");
  pysg_ospNodeType<OSPNodeLightsManager>(sg, "OSPNodeLightsManager");
  pysg_ospNodeType<OSPNodeData>(sg, "OSPNodeData");

  // SG Node classes //////////////////////////////////////////////////////////

  py::class_<Camera,
      OSPNode<ospray::cpp::Camera, NodeType::CAMERA>,
      Node,
      std::shared_ptr<Camera>>(sg, "Camera")
      .def(py::init<const std::string &>());

  py::class_<Frame,
      OSPNode<ospray::cpp::Future, NodeType::FRAME>,
      Node,
      std::shared_ptr<Frame>>(sg, "Frame")
      .def(py::init<>())
      .def("saveFrame", &Frame::saveFrame)
      .def("waitOnFrame", &Frame::waitOnFrame)
      .def("startNewFrame", &Frame::startNewFrame);

  py::class_<Renderer,
      OSPNode<ospray::cpp::Renderer, NodeType::RENDERER>,
      Node,
      std::shared_ptr<Renderer>>(sg, "Renderer")
      .def(py::init<const std::string &>());

  py::class_<FrameBuffer,
      OSPNode<ospray::cpp::FrameBuffer, NodeType::FRAME_BUFFER>,
      Node,
      std::shared_ptr<FrameBuffer>>(sg, "FrameBuffer")
      .def(py::init<>());

  py::class_<World,
      OSPNode<ospray::cpp::World, NodeType::WORLD>,
      Node,
      std::shared_ptr<World>>(sg, "World")
      .def(py::init<>());

  py::class_<Geometry,
      OSPNode<ospray::cpp::Geometry, NodeType::GEOMETRY>,
      Node,
      std::shared_ptr<Geometry>>(sg, "Geometry")
      .def(py::init<const std::string &>());

  py::class_<MaterialRegistry, Node, std::shared_ptr<MaterialRegistry>>(
      sg, "MaterialRegistry")
      .def(py::init<>());

  py::class_<LightsManager,
      OSPNode<ospray::cpp::Light, NodeType::LIGHTS>,
      Node,
      std::shared_ptr<LightsManager>>(sg, "LightsManager")
      .def(py::init<>());

  py::class_<Data,
      OSPNode<ospray::cpp::CopiedData, NodeType::PARAMETER>,
      Node,
      std::shared_ptr<Data>>(sg, "Data")
      .def(py::init<>());

}