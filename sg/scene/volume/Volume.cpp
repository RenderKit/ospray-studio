// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Volume.h"

namespace ospray {
  namespace sg {

  Volume::Volume(const std::string &osp_type)
  {
    setValue(cpp::Volume(osp_type));
    createChild("visible", "bool", true);
    createChild("filter", "int", "0 = nearest, 100 = trilinear", 0);
  }

  NodeType Volume::type() const
  {
    return NodeType::VOLUME;
  }

  void Volume::load(const FileName &){}

  }  // namespace sg
} // namespace ospray
