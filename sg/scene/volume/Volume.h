// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../../Node.h"
// ospcommon
#include "rkcommon/os/FileName.h"

namespace ospray {
  namespace sg {

  static std::unordered_map<std::string, OSPDataType> const volumeVoxelType = {
    {"float", OSP_FLOAT},
    {"int", OSP_INT},
    {"uchar", OSP_UCHAR},
    {"short", OSP_SHORT},
    {"ushort", OSP_USHORT},
    {"double", OSP_DOUBLE}};

  struct OSPSG_INTERFACE VolumeParams : public Node
  {
    VolumeParams() = default;
    VolumeParams(bool structured)
    {
      if (structured) {
        createChild("voxelType", "int", int(OSP_FLOAT));
        createChild("dimensions", "vec3i", vec3i(18, 25, 18));
        createChild("grdiOrigin", "vec3f", vec3f(-1.f));
        createChild("gridSpacing", "vec3f", vec3f(2.f / 100));
      } else {
        createChild("voxelType", "int", int(OSP_FLOAT));
        createChild("dimensions", "vec3i", vec3i(180, 180, 180));
        createChild("grdiOrigin", "vec3f", vec3f(0));
        createChild("gridSpacing", "vec3f", vec3f(1, 1, 1));
      }
    }
  };

  struct OSPSG_INTERFACE Volume : public OSPNode<cpp::Volume, NodeType::VOLUME>
  {
    Volume(const std::string &osp_type);
    ~Volume() override = default;

    NodeType type() const override;
    virtual void load(const FileName &fileName);
    
    private:
    bool fileLoaded{false};
  };

  }  // namespace sg
} // namespace ospray
