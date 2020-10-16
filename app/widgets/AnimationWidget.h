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
#include "../../sg/scene/lights/Lights.h"
#include "../../sg/scene/World.h"
#include "../../sg/fb/FrameBuffer.h"
#include "../../sg/visitors/PrintNodes.h"

using namespace rkcommon::math;

  struct AnimationParameters
  {
    int currentKeyframe           = 0;
    bool playKeyframes            = false;
    int desiredKeyframesPerSecond = 4;

    int animationCenter       = 0;
    int numTimesteps = 0;
    int animationIncrement    = 1;

    int computedAnimationMin = 0;
    int computedAnimationMax = 0;

    bool pauseRendering = false;
  };

class AnimationWidget
{
 public:
  AnimationWidget(std::shared_ptr<ospray::sg::Frame> activeFrame,
      ospray::sg::NodePtr firstWorld,
      std::shared_ptr<ospray::sg::Lights> _lightsManager,
      std::vector<float> &timesteps,
      const std::string &_widgetName);
  ~AnimationWidget();

  // update UI and process any UI events
  void addAnimationUI();

  void insertKeyframeMenu();

  // widget name (use different names to support multiple concurrent widgets)
  std::string widgetName;

  void setKeyframe(int numKeyframe);

  void animateKeyframes();

 private:
  AnimationParameters animationParameters;
  std::shared_ptr<ospray::sg::Frame> activeFrame;
  std::vector<std::shared_ptr<ospray::sg::World>> g_allWorlds;
  ospray::sg::NodePtr firstWorld;
  std::shared_ptr<ospray::sg::Lights> lightsManager;
  std::vector<float> &timesteps;
  int numKeyframes{0};
};
