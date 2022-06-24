// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "AnimationManager.h"
#include <imgui.h>

void AnimationManager::init()
{
  for (auto &a : animations)
    timeRange.extend(a.timeRange);

  if (time) {
    update(time, shutter);
  } else
    update(timeRange.lower);
}

void AnimationManager::update(const float _time, const float _shutter)
{
  for (auto &a : animations) {
    a.update(_time, _shutter);
    time = _time;
    shutter = _shutter;
  }
}
