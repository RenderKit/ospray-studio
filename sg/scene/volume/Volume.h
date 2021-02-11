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

  struct OSPSG_INTERFACE Volume : public OSPNode<cpp::Volume, NodeType::VOLUME>
  {
    Volume(const std::string &osp_type);
    ~Volume() override = default;

    NodeType type() const override;
    virtual void load(const FileName &fileName);
  };

  }  // namespace sg
} // namespace ospray
