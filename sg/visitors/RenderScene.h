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

#include "../Node.h"
// std
#include <stack>

namespace ospray::sg {

  struct RenderScene : public Visitor
  {
    RenderScene();

    bool operator()(Node &node, TraversalContext &ctx) override;
    void postChildren(Node &node, TraversalContext &) override;

   private:
    // Helper Functions //
    void createGeometry(Node &node);
    void createVolume(Node &node);
    void addGeometriesToGroup();
    void createInstanceFromGroup();
    void placeInstancesInWorld();
    void addLightToWorld(Node &node);

    // Data //

    struct
    {
      std::vector<cpp::GeometricModel> geometries;
      std::vector<cpp::VolumetricModel> volumes;
      cpp::Group group;
      // Apperance information:
      //     - Material
      //     - TransferFunction
      //     - ...others?
    } current;
    cpp::World world;
    std::vector<cpp::Instance> instances;
    std::stack<affine3f> xfms;
    std::stack<uint32_t> materialIDs;
    std::stack<cpp::TransferFunction> tfns;
  };

  // Inlined definitions //////////////////////////////////////////////////////

  inline RenderScene::RenderScene()
  {
    xfms.emplace(math::one);
    materialIDs.push(0);
  }

  inline bool RenderScene::operator()(Node &node, TraversalContext &)
  {
    bool traverseChildren = true;

    switch (node.type()) {
    case NodeType::WORLD:
      world = node.valueAs<cpp::World>();
      break;
    case NodeType::GEOMETRY:
      createGeometry(node);
      traverseChildren = false;
      break;
    case NodeType::VOLUME:
      createVolume(node);
      traverseChildren = false;
      break;
    case NodeType::TRANSFER_FUNCTION:
      tfns.push(node.valueAs<cpp::TransferFunction>());
      break;
    case NodeType::TRANSFORM:
      xfms.push(xfms.top() * node.valueAs<affine3f>());
      break;
    case NodeType::MATERIAL_REFERENCE:
      materialIDs.push(node.valueAs<int>());
      break;
    case NodeType::LIGHT:
      addLightToWorld(node);
      break;
    default:
      break;
    }

    return traverseChildren;
  }

  inline void RenderScene::postChildren(Node &node, TraversalContext &)
  {
    switch (node.type()) {
    case NodeType::WORLD:
      createInstanceFromGroup();
      placeInstancesInWorld();
      world.commit();
      break;
    case NodeType::TRANSFER_FUNCTION:
      tfns.pop();
      break;
    case NodeType::TRANSFORM:
      createInstanceFromGroup();
      xfms.pop();
      break;
    case NodeType::MATERIAL_REFERENCE:
      materialIDs.pop();
      break;
    case NodeType::LIGHT:
      world.commit();
    default:
      // Nothing
      break;
    }
  }

  inline void RenderScene::createGeometry(Node &node)
  {
    auto geom = node.valueAs<cpp::Geometry>();
    cpp::GeometricModel model(geom);
    if (node.hasChild("material")) {
      model.setParam("material", node["material"].valueAs<cpp::Data>());
    } else {
      model.setParam("material", materialIDs.top());
    }
    model.commit();
    current.geometries.push_back(model);
  }

  inline void RenderScene::createVolume(Node &node)
  {
    auto vol = node.valueAs<cpp::Volume>();
    cpp::VolumetricModel model(vol);
    if (node.hasChild("transfer_function")) {
      model.setParam(
          "transfer_function",
          node["transfer_function"].valueAs<cpp::TransferFunction>());
    } else {
      model.setParam("transferFunction", tfns.top());
    }
    model.commit();
    current.volumes.push_back(model);
  }

  inline void RenderScene::createInstanceFromGroup()
  {
    if (current.geometries.empty() && current.volumes.empty())
      return;

    if (!current.geometries.empty())
      current.group.setParam("geometry", cpp::Data(current.geometries));

    if (!current.volumes.empty())
      current.group.setParam("volume", cpp::Data(current.volumes));

    current.geometries.clear();
    current.volumes.clear();

    current.group.commit();

    cpp::Instance inst(current.group);
    inst.setParam("xfm", xfms.top());
    inst.commit();
    instances.push_back(inst);
  }

  inline void RenderScene::addLightToWorld(Node &node)
  {
    auto &light = node.valueAs<cpp::Light>();
    world.setParam("light", (cpp::Data)light);
  }

  inline void RenderScene::placeInstancesInWorld()
  {
    if (!instances.empty())
      world.setParam("instance", cpp::Data(instances));
  }

}  // namespace ospray::sg
