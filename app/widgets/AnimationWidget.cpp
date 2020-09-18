// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "AnimationWidget.h"
#include <imgui.h>
// std
#include <chrono>

AnimationWidget::AnimationWidget(std::shared_ptr<ospray::sg::Frame> activeFrame,
    ospray::sg::NodePtr firstWorld,
    std::vector<float> &timesteps,
    const std::string &_widgetName)
    : activeFrame(activeFrame),
      firstWorld(firstWorld),
      timesteps(timesteps),
      widgetName(_widgetName)
{
  numKeyframes = timesteps.size();
  firstWorld->render();
  activeFrame->add(firstWorld);

  // traverse and collect the animation worlds
  for (auto iter = timesteps.begin(); iter != timesteps.end(); ++iter) {
    auto newWorld = ospray::sg::createNode("world", "world");
    newWorld->createChild("time", "float", *iter);
    firstWorld->traverseAnimation<ospray::sg::GenerateAnimationTimeline>(
        newWorld);
    g_allWorlds.push_back(
        std::static_pointer_cast<ospray::sg::World>(newWorld));

    newWorld->render();
  }
}

AnimationWidget::~AnimationWidget() {}

// update UI and process any UI events
void AnimationWidget::addAnimationUI()
{
  ImGui::Begin(widgetName.c_str());

  animateKeyframes();

  insertKeyframeMenu();

  ImGui::End();
}

void AnimationWidget::insertKeyframeMenu()
{
  int numTimesteps = numKeyframes;

  if (g_animationParameters.playKeyframes) {
    if (ImGui::Button("Pause")) {
      g_animationParameters.playKeyframes = false;
    }
  } else {
    if (ImGui::Button("Play")) {
      g_animationParameters.playKeyframes = true;
    }
  }

  ImGui::SameLine();

  if (ImGui::SliderInt("Keyframe",
          &g_animationParameters.currentKeyframe,
          0,
          numTimesteps - 1)) {
    setKeyframe(g_animationParameters.currentKeyframe);
  }

  // set numTimesteps in animation params equal to numTimesteps of widget
  // constructor
  if (g_animationParameters.numTimesteps == 0) {
    g_animationParameters.numTimesteps = numTimesteps;
  }

  ImGui::SliderInt("Number of Keyframes",
      &g_animationParameters.numTimesteps,
      1,
      numTimesteps - 1);

  ImGui::SliderInt(
      "Animation increment", &g_animationParameters.animationIncrement, 1, 16);

  ImGui::SliderInt("Animation center",
      &g_animationParameters.animationCenter,
      0,
      numTimesteps - 1);

  // compute actual animation bounds
  g_animationParameters.computedAnimationMin =
      g_animationParameters.animationCenter
      - 0.5 * g_animationParameters.numTimesteps
          * g_animationParameters.animationIncrement;

  g_animationParameters.computedAnimationMax =
      g_animationParameters.animationCenter
      + 0.5 * g_animationParameters.numTimesteps
          * g_animationParameters.animationIncrement;

  if (g_animationParameters.computedAnimationMin < 0) {
    int delta = -g_animationParameters.computedAnimationMin;
    g_animationParameters.computedAnimationMin = 0;
    g_animationParameters.computedAnimationMax += delta;
  }

  if (g_animationParameters.computedAnimationMax > numTimesteps - 1) {
    int delta = g_animationParameters.computedAnimationMax - (numTimesteps - 1);
    g_animationParameters.computedAnimationMax = numTimesteps - 1;
    g_animationParameters.computedAnimationMin -= delta;
  }

  g_animationParameters.computedAnimationMin =
      std::max(0, g_animationParameters.computedAnimationMin);
  g_animationParameters.computedAnimationMax =
      std::min(numTimesteps - 1, g_animationParameters.computedAnimationMax);

  ImGui::Text("animation range: %d -> %d",
      g_animationParameters.computedAnimationMin,
      g_animationParameters.computedAnimationMax);

  ImGui::Spacing();

  ImGui::End();
}

void AnimationWidget::animateKeyframes()
{
  int numTimesteps = numKeyframes;

  if (numTimesteps != 1 && g_animationParameters.playKeyframes) {
    static auto keyframeLastChanged = std::chrono::system_clock::now();
    double minChangeInterval =
        1. / double(g_animationParameters.desiredKeyframesPerSecond);
    auto now = std::chrono::system_clock::now();

    std::chrono::duration<double> elapsedSeconds = now - keyframeLastChanged;

    if (elapsedSeconds.count() > minChangeInterval) {
      g_animationParameters.currentKeyframe +=
          g_animationParameters.animationIncrement;

      if (g_animationParameters.currentKeyframe
          < g_animationParameters.computedAnimationMin) {
        g_animationParameters.currentKeyframe =
            g_animationParameters.computedAnimationMin;
      }

      if (g_animationParameters.currentKeyframe
          > g_animationParameters.computedAnimationMax) {
        g_animationParameters.currentKeyframe =
            g_animationParameters.computedAnimationMin;
      }

      setKeyframe(g_animationParameters.currentKeyframe);
      keyframeLastChanged = now;
    }
  }
}

void AnimationWidget::setKeyframe(int keyframe)
{
  auto &world = g_allWorlds[keyframe];
  activeFrame->add(world);

  activeFrame->childAs<ospray::sg::FrameBuffer>("framebuffer")
      .resetAccumulation();
}

// stack implementation of keyframes
