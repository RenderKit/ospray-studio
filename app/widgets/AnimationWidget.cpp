// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "AnimationWidget.h"
#include <imgui.h>

AnimationWidget::AnimationWidget(
    std::string name, std::shared_ptr<AnimationManager> animationManager)
    : name(name), animationManager(animationManager)
{
  lastUpdated = std::chrono::system_clock::now();
  if (!animationManager->getTime()) {
    time = animationManager->getTimeRange().lower;
  } else {
    time = animationManager->getTime();
    shutter = animationManager->getShutter();
  }

  animationManager->update(time, shutter);
}

AnimationWidget::~AnimationWidget() {
  animationManager.reset();
}

void AnimationWidget::update()
{
  auto &timeRange = animationManager->getTimeRange();
  auto now = std::chrono::system_clock::now();
  if (play) {
    time += std::chrono::duration<float>(now - lastUpdated).count() * speedup;
    if (time > timeRange.upper)
      if (loop) {
        const float d = timeRange.size();
        time =
            d == 0.f ? timeRange.lower : timeRange.lower + std::fmod(time, d);
      } else {
        time = timeRange.lower;
        play = false;
      }
  }
  animationManager->update(time, shutter);
  lastUpdated = now;
}

// update UI and process any UI events
void AnimationWidget::addUI()
{
  if (!showUI)
    return;

  auto &timeRange = animationManager->getTimeRange();
  auto &animations = animationManager->getAnimations();

  ImGui::Begin(name.c_str(), &showUI);

  if (animationManager->getAnimations().empty()) {
    ImGui::Text("== No animated objects in the scene ==");
    ImGui::End();
    return;
  }

  if (ImGui::Button(play ? "Pause" : "Play ")) {
    play = !play;
    lastUpdated = std::chrono::system_clock::now();
  }

  bool modified = play;

  ImGui::SameLine();
  if (ImGui::SliderFloat("time", &time, timeRange.lower, timeRange.upper))
    modified = true;

  ImGui::SameLine();
  ImGui::Checkbox("Loop", &loop);

  { // simulate log slider
    static float exp = 0.f;
    ImGui::SliderFloat("speedup: ", &exp, -3.f, 3.f);
    speedup = std::pow(10.0f, exp);
    ImGui::SameLine();
    ImGui::Text("%.*f", std::max(0, int(1.99f - exp)), speedup);
  }

  if (ImGui::SliderFloat("shutter", &shutter, 0.0f, timeRange.size()))
    modified = true;

  for (auto &a : animations)
    ImGui::Checkbox(a.name.c_str(), &a.active);

  ImGui::Spacing();
  ImGui::End();

  if (modified)
    update();
}
