// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "AnimationWidget.h"
#include <imgui.h>

AnimationWidget::AnimationWidget(std::shared_ptr<ospray::sg::Frame> activeFrame,
    std::vector<ospray::sg::Animation> &animations,
    const std::string &name)
    : name(name), activeFrame(activeFrame), animations(animations)
{
  for (auto &a : animations)
    timeRange.extend(a.timeRange);
  time = timeRange.lower;
  lastUpdated = std::chrono::system_clock::now();
}

// update UI and process any UI events
void AnimationWidget::addAnimationUI()
{
  ImGui::Begin(name.c_str());

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
    ImGui::Text("%.*f", speedup, std::max(0, int(1.99f - exp)));
  }

  for (auto &a : animations)
    ImGui::Checkbox(a.name.c_str(), &a.active);

  ImGui::Spacing();
  ImGui::End();

  if (modified)
    update();
}

void AnimationWidget::update()
{
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
  lastUpdated = now;

  for (auto &a : animations)
    a.update(time);
}

// stack implementation of keyframes
