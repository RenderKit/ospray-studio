// Copyright 2009-2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../../Node.h"

namespace ospray {
namespace sg {

using GroupPtr = std::shared_ptr<cpp::Group>;
using GeometricModelPtr = std::shared_ptr<cpp::GeometricModel>;

struct OSPSG_INTERFACE Skin
{
  std::vector<affine3f> inverseBindMatrices;
  std::vector<NodePtr> joints;
};
using SkinPtr = std::shared_ptr<Skin>;

struct OSPSG_INTERFACE Geometry
    : public OSPNode<cpp::Geometry, NodeType::GEOMETRY>
{
  Geometry(const std::string &osp_type);
  ~Geometry() override = default;

  NodeType type() const override;

  virtual void postCommit() override;

  // skinning
  bool checkAndNormalizeWeights();
  SkinPtr skin;
  NodePtr skeletonRoot;
  size_t weightsPerVertex{0};
  std::vector<uint16_t> joints; // 2D
  std::vector<float> weights; // 2D
  std::vector<vec3f> positions;
  std::vector<vec3f> skinnedPositions;
  std::vector<vec3f> skinnedEndPositions;
  std::vector<vec3f> normals;
  std::vector<vec3f> skinnedNormals;
  std::vector<vec3f> skinnedEndNormals;

  // XXX: Create node types based on actual accessor types
  std::vector<vec3ui> vi; // XXX support both 3i and 4i OSPRay 2?
  std::vector<vec4ui> quad_vi;
  std::vector<vec4f> vc;
  std::vector<vec2f> vt;
  std::vector<uint32_t> mIDs;

  GroupPtr group{nullptr};
  GeometricModelPtr model{nullptr};
};

} // namespace sg
} // namespace ospray
