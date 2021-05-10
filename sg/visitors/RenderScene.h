// Copyright 2009-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "sg/Node.h"
#include "sg/renderer/MaterialRegistry.h"
#include "sg/scene/Transform.h"
#include "sg/scene/geometry/Geometry.h"
#include "sg/scene/lights/Light.h"
#include "sg/camera/Camera.h"
#include "app/ArcballCamera.h"
// std
#include <stack>

namespace ospray {
  namespace sg {

  struct RenderScene : public Visitor
  {
    RenderScene();
    RenderScene(GeomIdMap &geomIdMap, InstanceIdMap &instanceIdMap, affine3f *cameraToWorld, int cId);

    bool operator()(Node &node, TraversalContext &ctx) override;
    void postChildren(Node &node, TraversalContext &) override;

   private:
    // Helper Functions //
    void createGeometry(Node &node);
    void createVolume(Node &node);
    void addGeometriesToGroup();
    void createInstanceFromGroup(Node &node);
    void placeInstancesInWorld();
    void setLightParams(Node &node);
    void setCameraParams(Node &node);

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
    std::vector<cpp::Instance> instances;
    std::stack<affine3f> xfms;
    std::stack<uint32_t> materialIDs;
    std::stack<cpp::TransferFunction> tfns;
    std::string instanceId{""};
    std::string geomId{""};
    GeomIdMap *g{nullptr};
    InstanceIdMap *in{nullptr};
    affine3f *camXfm{nullptr};
    int camId{0};
    std::shared_ptr<CameraState> cs;
  };

  // Inlined definitions //////////////////////////////////////////////////////

  inline RenderScene::RenderScene()
  {
    xfms.emplace(math::one);
  }

  inline RenderScene::RenderScene(GeomIdMap &geomIdMap, InstanceIdMap &instanceIdMap, affine3f *cameraToWorld, int cId)
  {
    xfms.emplace(math::one);
    g = &geomIdMap;
    in = &instanceIdMap;
    camXfm = cameraToWorld;
    camId = cId;
  }

  inline bool RenderScene::operator()(Node &node, TraversalContext &)
  {
    bool traverseChildren = true;

    switch (node.type()) {
    case NodeType::WORLD:
      world = node.valueAs<cpp::World>();
      break;
    case NodeType::MATERIAL_REFERENCE:
      materialIDs.push(node.valueAs<int>());
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
    case NodeType::TRANSFORM: {
      affine3f xfm =
          affine3f::rotate(node.child("rotation").valueAs<quaternionf>())
          * affine3f::scale(node.child("scale").valueAs<vec3f>());
      xfm.p = node.child("translation").valueAs<vec3f>();
      auto xfmNode = node.nodeAs<Transform>();
      xfmNode->accumulatedXfm = xfms.top() * xfm * node.valueAs<affine3f>();
      xfms.push(xfmNode->accumulatedXfm);

      if (node.hasChildOfSubType("camera_perspective")
          || node.hasChildOfSubType("camera_orthographic")) {
        cs = std::make_shared<CameraState>();
        cs->useCameraToWorld = true;
        cs->cameraToWorld = xfm;
      }

      if (node.hasChild("instanceId"))
        instanceId = node.child("instanceId").valueAs<std::string>();
      if (node.hasChild("geomId"))
        geomId = node.child("geomId").valueAs<std::string>();
      break;
    }
    case NodeType::LIGHT:
      setLightParams(node);
      break;
    case NodeType::CAMERA: {
      bool useCameraXfm{false};
      if (node.hasChild("cameraId"))
        useCameraXfm = camId == node.child("cameraId").valueAs<int>();
      if (camXfm && useCameraXfm) {
        auto &outputCamXfm = *camXfm;
        outputCamXfm = outputCamXfm * xfms.top();
      }
      setCameraParams(node);
    } break;
    default:
      break;
    }

    return traverseChildren;
  }

  inline void RenderScene::postChildren(Node &node, TraversalContext &)
  {
    switch (node.type()) {
    case NodeType::WORLD:
      placeInstancesInWorld();
      world.commit();
      break;
    case NodeType::GEOMETRY:
      createGeometry(node);
      break;
    case NodeType::TRANSFER_FUNCTION:
      tfns.pop();
      break;
    case NodeType::TRANSFORM:
      createInstanceFromGroup(node);
      xfms.pop();
      if (node.hasChild("instanceID")) {
          instanceId = "";
      }
      if (node.hasChild("geomId"))
        geomId = "";
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

    // skinning
    auto geomNode = node.nodeAs<Geometry>();
    if (geomNode->skin) {
      auto &joints = geomNode->skin->joints;
      auto &inverseBindMatrices = geomNode->skin->inverseBindMatrices;
      const size_t weightsPerVertex = geomNode->weightsPerVertex;
      size_t weightIdx = 0;
      for (size_t i = 0; i < geomNode->positions.size(); ++i) { // XXX parallel
        affine3f xfm{zero};
        for (size_t j = 0; j < weightsPerVertex; ++j, ++weightIdx) {
          const int idx = geomNode->joints[weightIdx];
          xfm = xfm
              + geomNode->weights[weightIdx]
                  * joints[idx]->nodeAs<Transform>()->accumulatedXfm
                  * inverseBindMatrices[idx];
        }
        xfm = rcp(geomNode->skeletonRoot->nodeAs<Transform>()->accumulatedXfm)
            * xfm;
        geomNode->skinnedPositions[i] = xfmPoint(xfm, geomNode->positions[i]);
        if (geomNode->skinnedNormals.size())
          geomNode->skinnedNormals[i] = xfmNormal(xfm, geomNode->normals[i]);
      }
    }

    auto geom = node.valueAs<cpp::Geometry>();
    cpp::GeometricModel model(geom);
    auto ospGeometricModel = model.handle();

    if (g != nullptr) {
      // if geometric Id was set by a parent transform node
      // this happens in very specific cases like BIT reference extensions
      if (!geomId.empty())
        g->insert(GeomIdMap::value_type(ospGeometricModel, geomId));
      else {
        // add geometry SG node name as unique geometry ID 
        g->insert(GeomIdMap::value_type(ospGeometricModel, node.name()));
      }
    }

    if (node.hasChild("material")) {
      if (node["material"].valueIsType<cpp::CopiedData>())
        model.setParam("material", node["material"].valueAs<cpp::CopiedData>());
      else
        model.setParam("material",
            cpp::CopiedData(std::vector<uint32_t>{
                node["material"].valueAs<unsigned int>()}));
    } else {
      if (current.materials.size() != 0) {
        model.setParam("material", cpp::CopiedData(*current.materials.begin()));
        current.materials.clear();
      } else
        model.setParam("material", cpp::CopiedData(std::vector<uint32_t>{0}));
    }

    if (node.hasChild("color")) {
      if (node["color"].valueIsType<cpp::CopiedData>())
        model.setParam("color", node["color"].valueAs<cpp::CopiedData>());
      else
        model.setParam("color",
            cpp::CopiedData(std::vector<vec4f>{
                node["color"].valueAs<vec4f>()}));
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

  inline void RenderScene::createInstanceFromGroup(Node &node)
  {
    #if defined(DEBUG)
    std::cout << "number of geometries : " << current.geometries.size()
              << std::endl;
    #endif

    if (current.geometries.empty() && current.volumes.empty()
        && current.clippingGeometries.empty())
      return;

    cpp::Group group;
    group.setParam("dynamicScene", node.child("dynamicScene").valueAs<bool>());
    group.setParam("compactMode", node.child("compactMode").valueAs<bool>());
    group.setParam("robustMode", node.child("robustMode").valueAs<bool>());

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

    if (in != nullptr && !instanceId.empty()) {
      auto ospInstance = inst.handle();
      in->insert(InstanceIdMap::value_type(ospInstance, std::make_pair(instanceId, xfms.top())));
    }
    
    #if defined(DEBUG)
    std::cout << "number of instances : " << instances.size() << std::endl;
    #endif
  }

  inline void RenderScene::setLightParams(Node &node)
  {
    auto type = node.subType();
    std::map<std::string, vec3f> propMap;

    if (type == "sphere" || type == "spot" || type == "quad") {
      auto lightPos = xfmPoint(xfms.top(), vec3f(0));
      propMap.insert(std::make_pair("position", lightPos));
    }

    if (type == "distant" || type == "hdri" || type == "spot") {
      auto lightDir = xfmVector(xfms.top(), vec3f(0, 0, 1));
      propMap.insert(std::make_pair("direction", lightDir));
    }

    if (type == "hdri") {
      auto upDir = xfmVector(xfms.top(), vec3f(0, 1, 0));
      propMap.insert(std::make_pair("up", upDir));
    }
    auto lightNode = node.nodeAs<sg::Light>();
    lightNode->initOrientation(propMap);
  }

  inline void RenderScene::setCameraParams(Node &node)
  {
    auto camera = node.nodeAs<sg::Camera>();
    if (cs != nullptr) {
      camera->setState(cs);
      if (node.parents().front()->child("animateCamera").valueAs<bool>())
        camera->animate = true;
    }
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
