// Copyright 2009-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../../Node.h"

namespace ospray {
  namespace sg {

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

    // skinning info
    SkinPtr skin;
    NodePtr skeletonRoot;
    size_t weightsPerVertex{0};
    std::vector<uint16_t> joints; // 2D
    std::vector<float> weights; // 2D
    std::vector<vec3f> positions;
    std::vector<vec3f> skinnedPositions;
    std::vector<vec3f> normals;
    std::vector<vec3f> skinnedNormals;
  };

  }  // namespace sg
} // namespace ospray
