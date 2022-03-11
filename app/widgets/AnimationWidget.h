// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once
#include <string>
#include "../AnimationManager.h"

class AnimationWidget
{
 public:
  AnimationWidget(
      std::string name, std::shared_ptr<AnimationManager> animationManager);
  ~AnimationWidget();
  void addAnimationUI();

  float getShutter()
  {
    return shutter;
  }

  bool showUI{true};

 private:
  bool play{false};
  bool loop{true};
  float speedup{1.0f};
  std::string name{"Animation Widget"};
  std::shared_ptr<AnimationManager> animationManager;
  std::chrono::time_point<std::chrono::system_clock> lastUpdated;
  float time{0.0f};
  float shutter{0.0f};
  void update();
};
