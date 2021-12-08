// Copyright 2009-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "sg/Node.h"
#include "sg/renderer/MaterialRegistry.h"
#include "sg/scene/Transform.h"
#include "sg/scene/geometry/Geometry.h"
#include "sg/scene/lights/Light.h"
#include "sg/camera/Camera.h"
#include "sg/scene/volume/Volume.h"
#include "sg/scene/World.h"
// std
#include <stack>

namespace ospray {
  namespace sg {

  struct RenderScene : public Visitor
  {
    RenderScene();
    ~RenderScene();

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
    std::shared_ptr<NodeInstanceMap> instMap{nullptr};
    Node *instRoot;
  };

  // Inlined definitions //////////////////////////////////////////////////////

  inline RenderScene::RenderScene()
  {
    xfms.emplace(math::one);
    endXfms.emplace(math::one);
    xfmsDiverged.emplace(false);
    if (instMap)
    instMap->clear();
  }

  inline RenderScene::~RenderScene()
  {
    groups.clear();
  }

  inline bool RenderScene::operator()(Node &node, TraversalContext &)
  {
    bool traverseChildren = true;

    switch (node.type()) {
    case NodeType::WORLD: {
      world = node.valueAs<cpp::World>();
      auto worldNode = node.nodeAs<World>();
      instMap = worldNode->instMap;
    } break;
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
      xfmNode->localXfm = xfm * node.valueAs<affine3f>();
      xfmNode->accumulatedXfm = xfms.top() * xfmNode->localXfm;
      xfms.push(xfmNode->accumulatedXfm);
      xfmNode->localEndXfm = endXfm * node.valueAs<affine3f>();
      xfmNode->accumulatedEndXfm = endXfms.top() * xfmNode->localEndXfm;
      endXfms.push(xfmNode->accumulatedEndXfm);
      xfmNode->motionBlur = xfmsDiverged.top() || diverged;
      xfmsDiverged.push(xfmNode->motionBlur);

      if (node.hasChild("hasReference")) 
        instRoot = &node;
      break;
    }
    case NodeType::CAMERA: {
      // camera transformation update
      auto &cam = node.valueAs<cpp::Camera>();
      if (xfmsDiverged.top()) { // motion blur
        std::vector<affine3f> motionXfms;
        motionXfms.push_back(xfms.top());
        motionXfms.push_back(endXfms.top());
        cam.removeParam("transform");
        cam.setParam("motion.transform", cpp::CopiedData(motionXfms));
      } else {
        cam.removeParam("motion.transform");
        cam.setParam("transform", xfms.top());
      }
      cam.commit();

      auto cameraNode = node.nodeAs<Camera>();
      cameraNode->cameraToWorld = xfms.top();
      cameraNode->cameraToWorldEnd = endXfms.top();
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
    case NodeType::LIGHT: {
      auto &light = node.valueAs<cpp::Light>();
      cpp::Group group;
      group.setParam("light", cpp::CopiedData(light));
      group.commit();
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
    } break;
    case NodeType::TRANSFORM:
      createInstanceFromGroup(node);
      xfms.pop();
      endXfms.pop();
      xfmsDiverged.pop();
      if (node.hasChild("hasReference"))
        instRoot = nullptr;
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
      geomNode->skinnedEndPositions.resize(geomNode->skinnedPositions.size());
      geomNode->skinnedEndNormals.resize(geomNode->skinnedNormals.size());
      size_t weightIdx = 0;

      // only worth with huge meshes:
      // tasking::parallel_in_blocks_of<64>(geomNode->positions.size(),
      // [&](size_t start, size_t end)
      for (size_t i = 0; i < geomNode->positions.size(); ++i) {
        affine3f xfm{zero};
        affine3f endXfm{zero};
        for (size_t j = 0; j < weightsPerVertex; ++j, ++weightIdx) {
          const int idx = geomNode->joints[weightIdx];
          // skinning matrix
          xfm = xfm
              + geomNode->weights[weightIdx]
                  * joints[idx]->nodeAs<Transform>()->accumulatedXfm
                  * inverseBindMatrices[idx];
          endXfm = endXfm
              + geomNode->weights[weightIdx]
                  * joints[idx]->nodeAs<Transform>()->accumulatedEndXfm
                  * inverseBindMatrices[idx];
        }
        // from gltf docu:
        // final joint matrix = globalTransformOfNodeThatTheMeshIsAttachedTo^-1 * 
        //                         globalTransformOfJointNode(j) *
        //                         inverseBindMatrixForJoint(j)
        xfm = rcp(geomNode->skeletonRoot->nodeAs<Transform>()->accumulatedXfm)
            * xfm;
        endXfm =
            rcp(geomNode->skeletonRoot->nodeAs<Transform>()->accumulatedEndXfm)
            * endXfm;
        geomNode->skinnedPositions[i] = xfmPoint(xfm, geomNode->positions[i]);
        geomNode->skinnedEndPositions[i] =
            xfmPoint(endXfm, geomNode->positions[i]);
        if (geomNode->skinnedNormals.size()) {
          geomNode->skinnedNormals[i] = xfmNormal(xfm, geomNode->normals[i]);
          geomNode->skinnedEndNormals[i] =
              xfmNormal(endXfm, geomNode->normals[i]);
        }
      }

      bool motionBlur = geomNode->skeletonRoot->nodeAs<Transform>()->motionBlur;
      for (auto idx : geomNode->joints)
        motionBlur |= joints[idx]->nodeAs<Transform>()->motionBlur;

      auto &geom = geomNode->valueAs<cpp::Geometry>();
      if (motionBlur) {
        std::vector<cpp::SharedData> motionPos;
        motionPos.push_back(cpp::SharedData(geomNode->skinnedPositions));
        motionPos.push_back(cpp::SharedData(geomNode->skinnedEndPositions));
        geom.setParam("motion.vertex.position", cpp::CopiedData(motionPos));
        geom.removeParam("vertex.position");
        if (geomNode->skinnedNormals.size()) {
          std::vector<cpp::SharedData> motionNor;
          motionNor.push_back(cpp::SharedData(geomNode->skinnedNormals));
          motionNor.push_back(cpp::SharedData(geomNode->skinnedEndNormals));
          geom.setParam("motion.vertex.normal", cpp::CopiedData(motionNor));
          geom.removeParam("vertex.normal");
        }
      } else {
        geom.setParam("vertex.position", cpp::SharedData(geomNode->skinnedPositions));
        geom.removeParam("motion.vertex.position");
        if (geomNode->skinnedNormals.size()) {
          geom.setParam("vertex.normal", cpp::SharedData(geomNode->skinnedNormals));
          geom.removeParam("motion.vertex.normal");
        }
      }
      geom.commit();

      // Recommit the cpp::Group for skinned animation to take effect
      geomNode->group->commit();
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

      // sg picking
      if (instMap && instRoot) {
        auto ospInstance = inst.handle();
        instMap->insert(NodeInstanceMap::value_type(instRoot, std::make_pair(&node, ospInstance)));
      }
    };

    if (node.hasChildOfType(NodeType::GEOMETRY)) {
      auto &geomChildren = node.childrenOfType(NodeType::GEOMETRY);
#if defined(DEBUG)
      std::cout << "number of geometries : " << geomChildren.size() << std::endl;
#endif

      for (auto geom : geomChildren) {
        auto geomNode = geom->nodeAs<Geometry>();
        auto geomIdentifier = geomNode->groupIndex;
        if (geomIdentifier >= 0 && geomIdentifier < groups.size()) {
          auto &group = groups[geomIdentifier];
          setInstance(group);
        }
      }
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

  inline void RenderScene::placeInstancesInWorld()
  {
    if (!instances.empty())
      world.setParam("instance", cpp::CopiedData(instances));
    else
      world.removeParam("instance");
  }

  }  // namespace sg
} // namespace ospray
