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
#include "sg/scene/volume/Volume.h"
// std
#include <stack>

namespace ospray {
  namespace sg {

  struct RenderScene : public Visitor
  {
    RenderScene();
    ~RenderScene();
    RenderScene(GeomIdMap &geomIdMap,
        InstanceIdMap &instanceIdMap,
        affine3f *cameraToWorld,
        std::vector<NodePtr> &instanceXfms,
        std::vector<std::tuple<std::string, box3f, affine3f>> &_groupBBoxes,
        int cId);

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
      std::vector<cpp::Texture> textures;
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
    // string identifier associated with a geometry
    std::vector<cpp::Group> groups;
    int groupIndex{0};
    std::stack<affine3f> xfms;
    std::stack<affine3f> endXfms;
    std::stack<bool> xfmsDiverged;
    std::stack<uint32_t> materialIDs;
    std::stack<cpp::TransferFunction> tfns;
    std::string instanceId{""};
    std::string geomId{""};
    GeomIdMap *g{nullptr};
    InstanceIdMap *in{nullptr};
    affine3f *camXfm{nullptr};
    int camId{0};
    std::shared_ptr<CameraState> cs;
    std::vector<NodePtr> *instanceXfms{nullptr};
    std::vector<std::tuple<std::string, box3f, affine3f>> *groupBBoxes{nullptr};
    box3f currentInstBBox{empty};
  };

  // Inlined definitions //////////////////////////////////////////////////////

  inline RenderScene::RenderScene()
  {
    xfms.emplace(math::one);
    endXfms.emplace(math::one);
    xfmsDiverged.emplace(false);
  }

  inline RenderScene::~RenderScene()
  {
    groups.clear();
  }

  inline RenderScene::RenderScene(GeomIdMap &geomIdMap,
      InstanceIdMap &instanceIdMap,
      affine3f *cameraToWorld,
      std::vector<NodePtr> &_instanceXfms,
      std::vector<std::tuple<std::string, box3f, affine3f>> &_groupBBoxes,
      int cId)
  {
    xfms.emplace(math::one);
    endXfms.emplace(math::one);
    xfmsDiverged.emplace(false);
    g = &geomIdMap;
    in = &instanceIdMap;
    camXfm = cameraToWorld;
    camId = cId;
    instanceXfms = &_instanceXfms;
    groupBBoxes = &_groupBBoxes;
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
    case NodeType::GEOMETRY:
      createGeometry(node);
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
          affine3f::rotate(node.child("rotation").valueAs<quaternionf>());
      affine3f endXfm = xfm;
      bool diverged = false;
      if (node.child("rotation").hasChild("endKey")) {
        diverged = true;
        endXfm = affine3f::rotate(
            node.child("rotation").child("endKey").valueAs<quaternionf>());
      }

      const affine3f sxfm =
          affine3f::scale(node.child("scale").valueAs<vec3f>());
      xfm *= sxfm;
      if (node.child("scale").hasChild("endKey")) {
        diverged = true;
        endXfm *= affine3f::scale(
            node.child("scale").child("endKey").valueAs<vec3f>());
      } else
        endXfm *= sxfm;

      xfm.p = node.child("translation").valueAs<vec3f>();
      if (node.child("translation").hasChild("endKey")) {
        diverged = true;
        endXfm.p = node.child("translation").child("endKey").valueAs<vec3f>();
      } else
        endXfm.p = xfm.p;

      auto xfmNode = node.nodeAs<Transform>();
      xfmNode->accumulatedXfm = xfms.top() * xfm * node.valueAs<affine3f>();
      xfms.push(xfmNode->accumulatedXfm);
      endXfms.push(endXfms.top() * endXfm * node.valueAs<affine3f>());
      xfmsDiverged.push(xfmsDiverged.top() || diverged);

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
    case NodeType::TRANSFER_FUNCTION:
      tfns.pop();
      break;
    case NodeType::TRANSFORM:
      createInstanceFromGroup(node);
      xfms.pop();
      endXfms.pop();
      xfmsDiverged.pop();
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
    auto geomNode = node.nodeAs<Geometry>();

    if (!node.child("visible").valueAs<bool>()) {
      geomNode->groupIndex = -1;
      return;
    }

    if (geomNode->groupIndex >= 0 && geomNode->groupIndex < groups.size())
      return;

    // skinning
    if (geomNode->skin) {
      auto &joints = geomNode->skin->joints;
      auto &inverseBindMatrices = geomNode->skin->inverseBindMatrices;
      const size_t weightsPerVertex = geomNode->weightsPerVertex;
      size_t weightIdx = 0;

      for (size_t i = 0; i < geomNode->positions.size(); ++i) { // XXX parallel
        affine3f xfm{zero};
        for (size_t j = 0; j < weightsPerVertex; ++j, ++weightIdx) {
          const int idx = geomNode->joints[weightIdx];
          // skinning matrix
          xfm = xfm
              + geomNode->weights[weightIdx]
                  * joints[idx]->nodeAs<Transform>()->accumulatedXfm
                  * inverseBindMatrices[idx];
        }
        // from gltf docu:
        // final joint matrix = globalTransformOfNodeThatTheMeshIsAttachedTo^-1 * 
        //                         globalTransformOfJointNode(j) *
        //                         inverseBindMatrixForJoint(j)
        xfm = rcp(geomNode->skeletonRoot->nodeAs<Transform>()->accumulatedXfm)
            * xfm;
        geomNode->skinnedPositions[i] = xfmPoint(xfm, geomNode->positions[i]);
        if (geomNode->skinnedNormals.size())
          geomNode->skinnedNormals[i] = xfmNormal(xfm, geomNode->normals[i]);
      }

      // Recommit the cpp::Group for skinned animation to take effect
      geomNode->group->commit();
    }

    if (g != nullptr) {
      auto ospGeometricModel = geomNode->model->handle();
      // if geometric Id was set by a parent transform node
      // this happens in very specific cases like BIT reference extensions
      if (!geomId.empty())
        g->insert(GeomIdMap::value_type(ospGeometricModel, geomId));
      else {
        // add geometry SG node name as unique geometry ID 
        g->insert(GeomIdMap::value_type(ospGeometricModel, node.name()));
      }
    }

    groups.push_back(*geomNode->group);
    geomNode->groupIndex = groupIndex;
    groupIndex++;
  }

  inline void RenderScene::createVolume(Node &node)
  {
    auto volNode = node.nodeAs<sg::Volume>();

    if (!node.child("visible").valueAs<bool>()) {
      volNode->groupIndex = -1;
      return;
    }

    if (volNode->groupIndex >= 0 && volNode->groupIndex < groups.size())
      return;

    auto &vol = node.valueAs<cpp::Volume>();
    cpp::VolumetricModel model(vol);
    if (node.hasChildOfType(sg::NodeType::TRANSFER_FUNCTION)) {
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

    cpp::Group group;
    group.setParam("volume", cpp::CopiedData(model));

    group.commit();
    groups.push_back(group);
    volNode->groupIndex = groupIndex;
    groupIndex++;
  }

  inline void RenderScene::createInstanceFromGroup(Node &node)
  {
    #if defined(DEBUG)
    std::cout << "number of geometries : " << current.geometries.size()
              << std::endl;
    #endif

    auto setInstance = [&](auto group) {
      group.setParam(
          "dynamicScene", node.child("dynamicScene").valueAs<bool>());
      group.setParam("compactMode", node.child("compactMode").valueAs<bool>());
      group.setParam("robustMode", node.child("robustMode").valueAs<bool>());

      cpp::Instance inst(group);
      if (xfmsDiverged.top()) { // motion blur
        std::vector<affine3f> motionXfms;
        motionXfms.push_back(xfms.top());
        motionXfms.push_back(endXfms.top());
        inst.setParam("motion.transform", cpp::CopiedData(motionXfms));
      } else
        inst.setParam("transform", xfms.top());
      inst.commit();
      instances.push_back(inst);

      if (in != nullptr && !instanceId.empty()) {
        auto ospInstance = inst.handle();
        in->insert(InstanceIdMap::value_type(
            ospInstance, std::make_pair(instanceId, xfms.top())));
      }
    };

    if (node.hasChildOfType(NodeType::GEOMETRY)) {
      auto &geomChildren = node.childrenOfType(NodeType::GEOMETRY);
      for (auto geom : geomChildren) {
        auto geomNode = geom->nodeAs<Geometry>();
        auto geomIdentifier = geomNode->groupIndex;
        if (geomIdentifier >= 0 && geomIdentifier < groups.size()) {
          auto &group = groups[geomIdentifier];
          if (groupBBoxes && !node.hasChild("instanceId")) {
            auto box = group.getBounds<box3f>();
            affine3f xfm =  affine3f::scale(node.child("scale").valueAs<vec3f>());
            xfm.p = node.child("translation").valueAs<vec3f>();
            auto xfmdBox = xfmBounds(xfm, box);
            currentInstBBox.extend(xfmdBox);
          }
          setInstance(group);
        }
      }
    }

    if (groupBBoxes && node.hasChild("instanceId")) {
      groupBBoxes->push_back(std::make_tuple(node.name(), currentInstBBox, xfms.top()));
      currentInstBBox = empty;
    }

    if (node.hasChildOfType(NodeType::VOLUME)) {
      auto &volChildren = node.childrenOfType(NodeType::VOLUME);
      for (auto vol : volChildren) {
        auto volNode = vol->nodeAs<Volume>();
        auto volumeIdentifier = volNode->groupIndex;
        if (volumeIdentifier >= 0 && volumeIdentifier < groups.size()) {
          auto &group = groups[volumeIdentifier];
          setInstance(group);
        }
      }
    }

    if (node.hasChildOfType(NodeType::TRANSFER_FUNCTION)) {
      auto &tfnChildren = node.childrenOfType(NodeType::TRANSFER_FUNCTION);
      for (auto tfn : tfnChildren) {
        auto volChildren = tfn->childrenOfType(NodeType::VOLUME);
        for (auto vol : volChildren) {
          auto volNode = vol->nodeAs<sg::Volume>();
          auto volumeIdentifier = volNode->groupIndex;
          if (volumeIdentifier >= 0 && volumeIdentifier < groups.size()) {
            auto &group = groups[volumeIdentifier];
            setInstance(group);
          }
        }
      }
    }

#if defined(DEBUG)
      std::cout << "number of instances : " << instances.size() << std::endl;
#endif
  }

  inline void RenderScene::setLightParams(Node &node)
  {
    auto type = node.subType();
    // properties map for initializing light property values
    std::unordered_map<std::string, std::pair<vec3f, bool>> propMap;

    if (type == "sphere" || type == "spot" || type == "quad") {
      auto lightPos = xfmPoint(xfms.top(), vec3f(0));
      propMap.insert(
          std::make_pair("position", std::make_pair(lightPos, false)));
    }

    if (type == "distant" || type == "hdri" || type == "spot") {
      auto lightDir = xfmVector(xfms.top(), vec3f(0, 0, 1));
      propMap.insert(
          std::make_pair("direction", std::make_pair(lightDir, false)));
    }

    if (type == "sunSky") {
      auto sunDir = xfmVector(xfms.top(), vec3f(0, -1, 0));
      propMap.insert(std::make_pair("direction", std::make_pair(sunDir, true)));
    }

    if (type == "hdri") {
      auto upDir = xfmVector(xfms.top(), vec3f(0, 1, 0));
      propMap.insert(std::make_pair("up", std::make_pair(upDir, false)));
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
