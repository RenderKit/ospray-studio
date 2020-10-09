// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../Node.h"
#include "../renderer/MaterialRegistry.h"
// std
#include <stack>

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
      std::vector<cpp::GeometricModel> clippingGeometries;
      // make this a shared pointer instead of vector
      std::vector<cpp::Texture> textures;
      // make this a shared pointer instead of vector
      std::vector<cpp::Material> materials;
      // cpp::Group group;
      // Apperance information:
      //     - Material
      //     - TransferFunction
      //     - ...others?
    } current;
    bool setTextureVolume{false};
    cpp::World world;
    int unusedGeoms = 0;
    std::vector<cpp::Instance> instances;
    std::stack<affine3f> xfms;
    std::stack<uint32_t> materialIDs;
    std::stack<cpp::TransferFunction> tfns;
    float currentTimestep;
  };

  // Inlined definitions //////////////////////////////////////////////////////

  inline RenderScene::RenderScene()
  {
    xfms.emplace(math::one);
  }

  inline bool RenderScene::operator()(Node &node, TraversalContext &ctx)
  {
    bool traverseChildren = true;

    switch (node.type()) {
    case NodeType::WORLD:
      world = node.valueAs<cpp::World>();
      if(node.hasChild("time"))
        currentTimestep = node.child("time").valueAs<float>();
      break;
    case NodeType::MATERIAL_REFERENCE:
      materialIDs.push(node.valueAs<int>());
      break;
    case NodeType::GEOMETRY:
      createGeometry(node);
      traverseChildren = false;
      break;
    case NodeType::TEXTUREVOLUME:
      setTextureVolume = true;
      current.textures.push_back(node.valueAs<cpp::Texture>());
      break;
    case NodeType::VOLUME:
      createVolume(node);
      traverseChildren = false;
      break;
    case NodeType::TRANSFER_FUNCTION:
      tfns.push(node.valueAs<cpp::TransferFunction>());
      break;
    case NodeType::ANIMATION:
    {
      auto &nc = node.children();
      for (auto &c : nc) {
        auto timestep = c.second->child("timestep").valueAs<float>();
          if (timestep == currentTimestep) {
            xfms.push(xfms.top() * c.second->valueAs<affine3f>());
          }
      }
      traverseChildren = false;
    } break;
    case NodeType::TRANSFORM:
      if (unusedGeoms == current.geometries.size())
        xfms.push(xfms.top() * node.valueAs<affine3f>());
        else if (!node.hasChild("timestep")) {
          createInstanceFromGroup();
          xfms.push(xfms.top() * node.valueAs<affine3f>());
      }
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
      break;
    case NodeType::LIGHT:
      world.commit();
      break;
    default:
      // Nothing
      break;
    }
  }

  inline void RenderScene::createGeometry(Node &node)
  {
    if (!node.child("visible").valueAs<bool>())
      return;

    auto geom = node.valueAs<cpp::Geometry>();
    cpp::GeometricModel model(geom);
    if (node.hasChild("material")) {
      model.setParam("material", node["material"].valueAs<cpp::CopiedData>());
    } else {
      if (current.materials.size() != 0) {
        model.setParam("material", *current.materials.begin());
        current.materials.clear();
      } else
        model.setParam("material", materialIDs.top());
    }

    if (node.hasChild("color")) {
      model.setParam("color", node["color"].valueAs<cpp::CopiedData>());
    }

    model.commit();
    if (!node.child("isClipping").valueAs<bool>())
      current.geometries.push_back(model);
    else
      current.clippingGeometries.push_back(model);
  }

  inline void RenderScene::createVolume(Node &node)
  {
    if (!node.child("visible").valueAs<bool>())
      return;

    auto &vol = node.valueAs<cpp::Volume>();
    cpp::VolumetricModel model(vol);
    if (node.hasChild("transferFunction")) {
      model.setParam("transferFunction",
                     node["transferFunction"].valueAs<cpp::TransferFunction>());
    } else
      model.setParam("transferFunction", tfns.top());
    if (node.hasChild("densityScale"))
      model.setParam("densityScale", node["densityScale"].valueAs<float>());
    if (node.hasChild("anisotropy"))
      model.setParam("anisotropy", node["anisotropy"].valueAs<float>());
    if (node.hasChild("albedo"))
      model.setParam("albedo", node["albedo"].valueAs<vec3f>());
    if (node.hasChild("densityFadeDepth"))
      model.setParam("densityFadeDepth", node["densityFadeDepth"].valueAs<int>());
    if (node.hasChild("densityFade"))
      model.setParam("densityFade", node["densityFade"].valueAs<float>());
    if (node.hasChild("transmittanceDistanceScale"))
      model.setParam("transmittanceDistanceScale", node["transmittanceDistanceScale"].valueAs<float>());
    if (node.hasChild("maxPathDepth"))
      model.setParam("maxPathDepth", node["maxPathDepth"].valueAs<int>());
    model.commit();
    if (setTextureVolume) {
      auto &tex = *current.textures.begin();
      tex.setParam("volume", model);
      tex.commit();

      // fix the following by allowing material registry communication and,
      // adding a material directly in the material registry
      // whose material reference can be added to the geometry
      cpp::Material texmaterial("scivis", "obj");
      texmaterial.setParam("map_kd", tex);
      texmaterial.commit();
      current.materials.push_back(texmaterial);

      current.textures.clear();
      setTextureVolume = false;
    } else
      current.volumes.push_back(model);
  }

  inline void RenderScene::createInstanceFromGroup()
  {
    #if defined(DEBUG)
    std::cout << "number of geometries : " << current.geometries.size()
              << std::endl;
    #endif

    if (current.geometries.empty() && current.volumes.empty()
        && current.clippingGeometries.empty())
      return;

    cpp::Group group;

    if (!current.geometries.empty())
      group.setParam("geometry", cpp::CopiedData(current.geometries));

    if (!current.volumes.empty())
      group.setParam("volume", cpp::CopiedData(current.volumes));

    if (!current.clippingGeometries.empty())
      group.setParam(
          "clippingGeometry", cpp::CopiedData(current.clippingGeometries));

    current.geometries.clear();
    current.volumes.clear();
    current.clippingGeometries.clear();

    group.commit();

    cpp::Instance inst(group);
    inst.setParam("xfm", xfms.top());
    inst.commit();
    instances.push_back(inst);
    #if defined(DEBUG)
    std::cout << "number of instances : " << instances.size() << std::endl;
    #endif
  }

  inline void RenderScene::addLightToWorld(Node &node)
  {
    // XXX remove.
    std::cout << "!!!!! RenderScene::addLightToWorld: " << node.name() << std::endl;
    std::cout << "!!!!! deprecated.  Add lights to the lightsManager" << std::endl;
  //  auto &light = node.valueAs<cpp::Light>();
  //  world.setParam("light", (cpp::CopiedData)light);
  }

  inline void RenderScene::placeInstancesInWorld()
  {
    if (!instances.empty())
      world.setParam("instance", cpp::CopiedData(instances));
    else
      world.removeParam("instance");
  }

  }  // namespace sg
} // namespace ospray
