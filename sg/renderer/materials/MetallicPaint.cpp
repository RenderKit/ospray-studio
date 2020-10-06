// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "../Material.h"

namespace ospray {
  namespace sg {

  struct OSPSG_INTERFACE MetallicPaint : public Material
  {
    MetallicPaint();
    ~MetallicPaint() override = default;
  };

  OSP_REGISTER_SG_NODE_NAME(MetallicPaint, metallicPaint);

  // MetallicPaint definitions //////////////////////////////////////////////////

  MetallicPaint::MetallicPaint() : Material("metallicPaint")
  {

  }

  }  // namespace sg
} // namespace ospray
