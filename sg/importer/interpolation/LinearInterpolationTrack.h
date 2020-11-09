// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <vector>

#include "rkcommon/math/AffineSpace.h"

using namespace rkcommon::math;

namespace ospray {
namespace sg {

std::pair<float, affine3f> kfLerp(const std::pair<float, affine3f> &from,
    const std::pair<float, affine3f> &to,
    float currentStep)
{
  std::pair<float, affine3f> newKeyframe;
  newKeyframe.first =
      rkcommon::math::lerp<float>(currentStep, from.first, to.first);
  newKeyframe.second =
      rkcommon::math::lerp<affine3f>(currentStep, from.second, to.second);
  return newKeyframe;
}

void linearInterpolationTSTrack(std::vector<float> &kfInput,
    std::vector<affine3f> &kfOutput,
    float stepIncrement,
    std::map<float, affine3f> &keyframeTrack)

{
  for (auto i = 0; i < kfInput.size() - 1; ++i) {
    auto from = std::make_pair(kfInput[i], kfOutput[i]);
    auto to = std::make_pair(kfInput[i + 1], kfOutput[i + 1]);
    for (float step = 0.f; step <= 1.f; step += stepIncrement) {
      keyframeTrack.insert(kfLerp(from, to, step));
    }
  }
}

void linearInterpolationRotationsTrack(std::vector<float> &kfInput,
    std::vector<quaternionf> &rotations,
    float stepIncrement,
    std::map<float, affine3f> &keyframeTrack)
{
  for (auto i = 0; i < kfInput.size() - 1; ++i) {
    auto from = std::make_pair(kfInput[i], rotations[i]);
    auto to = std::make_pair(kfInput[i + 1], rotations[i + 1]);
    for (float step = 0.f; step <= 1.f; step += stepIncrement) {
      affine3f xfm{one};
      std::pair<float, affine3f> newKeyframe;
      newKeyframe.first =
          rkcommon::math::lerp<float>(step, from.first, to.first);
      auto rot = slerp(step, from.second, to.second);
      newKeyframe.second = affine3f(linear3f(rot)) * xfm;
      keyframeTrack.insert(newKeyframe);
    }
  }
}

} // namespace sg
} // namespace ospray
