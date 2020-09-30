// Copyright 2020 Intel Corporation
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
    firstWorld->traverseAnimation<ospray::sg::GenerateAnimationWorld>(
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

  if (animationParameters.playKeyframes) {
    if (ImGui::Button("Pause")) {
      animationParameters.playKeyframes = false;
    }
  } else {
    if (ImGui::Button("Play")) {
      animationParameters.playKeyframes = true;
    }
  }

  ImGui::SameLine();

  if (ImGui::SliderInt("Keyframe",
          &animationParameters.currentKeyframe,
          0,
          numTimesteps - 1)) {
    setKeyframe(animationParameters.currentKeyframe);
  }

  // set numTimesteps in animation params equal to numTimesteps of widget
  // constructor
  if (animationParameters.numTimesteps == 0) {
    animationParameters.numTimesteps = numTimesteps;
  }

  ImGui::SliderInt("Number of Keyframes",
      &animationParameters.numTimesteps,
      1,
      numTimesteps - 1);

  ImGui::SliderInt(
      "Animation increment", &animationParameters.animationIncrement, 1, 16);

  ImGui::SliderInt("Animation center",
      &animationParameters.animationCenter,
      0,
      numTimesteps - 1);

  // compute actual animation bounds
  animationParameters.computedAnimationMin =
      animationParameters.animationCenter
      - 0.5 * animationParameters.numTimesteps
          * animationParameters.animationIncrement;

  animationParameters.computedAnimationMax =
      animationParameters.animationCenter
      + 0.5 * animationParameters.numTimesteps
          * animationParameters.animationIncrement;

  if (animationParameters.computedAnimationMin < 0) {
    int delta = -animationParameters.computedAnimationMin;
    animationParameters.computedAnimationMin = 0;
    animationParameters.computedAnimationMax += delta;
  }

  if (animationParameters.computedAnimationMax > numTimesteps - 1) {
    int delta = animationParameters.computedAnimationMax - (numTimesteps - 1);
    animationParameters.computedAnimationMax = numTimesteps - 1;
    animationParameters.computedAnimationMin -= delta;
  }

  animationParameters.computedAnimationMin =
      std::max(0, animationParameters.computedAnimationMin);
  animationParameters.computedAnimationMax =
      std::min(numTimesteps - 1, animationParameters.computedAnimationMax);

  ImGui::Text("animation range: %d -> %d",
      animationParameters.computedAnimationMin,
      animationParameters.computedAnimationMax);

  ImGui::Spacing();
}

void AnimationWidget::animateKeyframes()
{
  int numTimesteps = numKeyframes;

  if (numTimesteps != 1 && animationParameters.playKeyframes) {
    static auto keyframeLastChanged = std::chrono::system_clock::now();
    double minChangeInterval =
        1. / double(animationParameters.desiredKeyframesPerSecond);
    auto now = std::chrono::system_clock::now();

    std::chrono::duration<double> elapsedSeconds = now - keyframeLastChanged;

    if (elapsedSeconds.count() > minChangeInterval) {
      animationParameters.currentKeyframe +=
          animationParameters.animationIncrement;

      if (animationParameters.currentKeyframe
          < animationParameters.computedAnimationMin) {
        animationParameters.currentKeyframe =
            animationParameters.computedAnimationMin;
      }

      if (animationParameters.currentKeyframe
          > animationParameters.computedAnimationMax) {
        animationParameters.currentKeyframe =
            animationParameters.computedAnimationMin;
      }

      setKeyframe(animationParameters.currentKeyframe);
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
