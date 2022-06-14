// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "TransferFunction.h"

namespace ospray {
namespace sg {

TransferFunction::TransferFunction(const std::string &osp_type)
{
  setValue(cpp::TransferFunction(osp_type));

  createChild("valueRange", "vec2f", vec2f(0.f, 1.f));
  colors = {vec3f(0.f), vec3f(1.f)};
  opacities = {0.f, 1.f};

  createChildData("color", colors);
  createChildData("opacity", opacities);
}

} // namespace sg
} // namespace ospray
