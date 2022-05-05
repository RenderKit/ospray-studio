// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <chrono>
#include <vector>
#include "../../sg/scene/Animation.h"

using namespace rkcommon::math;

class AnimationManager
{
 public:
  void update(const float time, const float shutter = 0.0f);

  range1f &getTimeRange()
  {
    return timeRange;
  }

  void setTimeRange(const range1f &_timeRange)
  {
    timeRange = _timeRange;
  }

  std::vector<ospray::sg::Animation> &getAnimations()
  {
    return animations;
  }

  void init();

 private:
  std::vector<ospray::sg::Animation> animations;
  range1f timeRange;
};
