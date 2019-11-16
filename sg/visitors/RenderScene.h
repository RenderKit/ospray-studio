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

#include "sg/Node.h"

namespace ospray {
  namespace sg {

    struct RenderScene : public Visitor
    {
      RenderScene();

      bool operator()(Node &node, TraversalContext &ctx) override;
      void postChildren(Node &node, TraversalContext &) override;

     private:
      // Helper Functions //

      void createGeometry(Node &node);
      void addGeometriesToGroup();
      void createInstanceFromGroup();
      void placeInstancesInWorld();

      // Data //

      struct
      {
        std::vector<cpp::GeometricModel> geometries;
        std::vector<cpp::Instance> instances;
        cpp::World world;
        cpp::Group group;
        affine3f xfm;
        // Apperance information:
        //     - Material
        //     - TransferFunction
        //     - ...others?
      } current;
    };

    // Inlined definitions ////////////////////////////////////////////////////

    inline RenderScene::RenderScene()
    {
      current.group = cpp::Group();
      current.xfm   = ospcommon::math::one;
    }

    inline bool RenderScene::operator()(Node &node, TraversalContext &)
    {
      bool traverseChildren = false;

      switch (node.type()) {
      case NodeType::WORLD:
        current.world    = node.valueAs<cpp::World>();
        traverseChildren = true;
        break;
      case NodeType::GEOMETRY:
        createGeometry(node);
        break;
      default:
        traverseChildren = true;
        break;
      }

      return traverseChildren;
    }

    inline void RenderScene::postChildren(Node &node, TraversalContext &)
    {
      switch (node.type()) {
      case NodeType::WORLD:
        if (!current.geometries.empty()) {
          addGeometriesToGroup();
          createInstanceFromGroup();
        }
        placeInstancesInWorld();
        current.world.commit();
        break;
      default:
        // Nothing
        break;
      }
    }

    inline void RenderScene::createGeometry(Node &node)
    {
      auto geom = node.valueAs<cpp::Geometry>();
      cpp::GeometricModel model(geom);
      // TODO: add material/color information
      model.commit();
      current.geometries.push_back(model);
    }

    inline void RenderScene::addGeometriesToGroup()
    {
      if (!current.geometries.empty())
        current.group.setParam("geometry", cpp::Data(current.geometries));
      current.group.commit();
    }

    inline void RenderScene::createInstanceFromGroup()
    {
      cpp::Instance inst(current.group);
      inst.setParam("xfm", current.xfm);
      inst.commit();
      current.instances.push_back(inst);
    }

    inline void RenderScene::placeInstancesInWorld()
    {
      if (!current.instances.empty())
        current.world.setParam("instance", cpp::Data(current.instances));
    }

  }  // namespace sg
}  // namespace ospray