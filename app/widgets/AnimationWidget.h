// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <array>
#include <functional>
#include <string>
#include <vector>

#include <GLFW/glfw3.h>
#ifdef _WIN32
#undef APIENTRY
#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_EXPOSE_NATIVE_WGL
#include <GLFW/glfw3native.h>
#endif

#include <ospray/ospray.h>
#include "rkcommon/math/vec.h"

#include "../../sg/Node.h"

#include "../../sg/Frame.h"
#include "../../sg/fb/FrameBuffer.h"
#include "../../sg/scene/Animation.h"
#include "../../sg/scene/World.h"
#include "../../sg/scene/lights/Lights.h"
#include "../../sg/visitors/PrintNodes.h"

// std
#include <chrono>
using namespace rkcommon::math;

class AnimationWidget
{
 public:
  AnimationWidget(std::shared_ptr<ospray::sg::Frame> activeFrame,
      std::vector<ospray::sg::Animation> &animations,
      const std::string &name);

  // update UI, called every frame
  void addAnimationUI();

  // widget name (use different names to support multiple concurrent widgets)
  std::string name;

 private:
  void update();

  bool play{false};
  bool loop{true};
  float time{0.0f};
  std::chrono::time_point<std::chrono::system_clock> lastUpdated;
  float speed{1.0f / 24};
  float speedup{1.0f};
  range1f timeRange;
  std::shared_ptr<ospray::sg::Frame> activeFrame;
  std::vector<ospray::sg::Animation> &animations;
};
