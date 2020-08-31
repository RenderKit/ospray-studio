// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Volume.h"

namespace ospray {
  namespace sg {

  struct OSPSG_INTERFACE UnstructuredVolume : public Volume
  {
    UnstructuredVolume();
    virtual ~UnstructuredVolume() override = default;
  };

  OSP_REGISTER_SG_NODE_NAME(UnstructuredVolume, volume_unstructured);

  // UnstructuredVolume definitions /////////////////////////////////////////////

  UnstructuredVolume::UnstructuredVolume() : Volume("unstructured")
  {
    createChildData("vertex.position");
    createChildData("index");
    createChildData("cell.index");
    createChildData("vertex.data");
    createChildData("cell.type");
  }

  }  // namespace sg
} // namespace ospray
