// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "AnimationManager.h"
#include <imgui.h>

void AnimationManager::init()
{
  for (auto &a : animations)
    timeRange.extend(a.timeRange);
}

void AnimationManager::update(float &time)
{
  for (auto &a : animations)
    a.update(time);
}
