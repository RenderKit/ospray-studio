// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "TransferFunction.h"

namespace ospray {
  namespace sg {

  TransferFunction::TransferFunction(const std::string &osp_type)
  {
    setValue(cpp::TransferFunction(osp_type));

    createChild("valueRange", "vec2f", vec2f(0.f, 1.f));
    std::vector<vec3f> colors = {vec3f(0.f), vec3f(1.f)};

    // createChild("valueRange", "vec2f", vec2f(-10000.f, 10000.f));

    // std::vector<vec3f> colors = {vec3f(1.0f, 0.0f, 0.0f),
    //                              vec3f(0.0f, 1.0f, 0.0f)};

    createChildData("color", colors);
    std::vector<float> opacities = {0.f, 1.f};
    createChildData("opacity", opacities);
  }

  }  // namespace sg
} // namespace ospray
