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
  void update(float &time);

  range1f &getTimeRange()
  {
    return timeRange;
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
