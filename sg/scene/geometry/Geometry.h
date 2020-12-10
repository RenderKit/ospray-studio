// Copyright 2009-2020 Intel Corporation
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
    std::vector<vec4us> joints;
    std::vector<vec4f> weights;
    std::vector<vec3f> positions;
    std::vector<vec3f> skinnedPositions;
    std::vector<vec3f> normals;
    std::vector<vec3f> skinnedNormals;
  };

  }  // namespace sg
} // namespace ospray
