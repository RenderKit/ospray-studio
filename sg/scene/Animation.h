// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../Node.h"

namespace ospray {
namespace sg {

typedef enum
{
  STEP,
  LINEAR,
  CUBIC
} InterpolationMode;

struct OSPSG_INTERFACE AnimationTrackBase
{
  virtual ~AnimationTrackBase() = default;
  virtual void update(const float time) = 0;

  InterpolationMode interpolation{InterpolationMode::STEP};
  NodePtr target;
  std::vector<float> times;

 protected:
  void updateIndex(const float time);
  size_t index{0}; // [i]<=time<[i+1], cache to avoid binary search
};

struct OSPSG_INTERFACE Animation
{
  Animation(const std::string &name);
  std::string name;
  bool active{true};
  range1f timeRange;

  void addTrack(AnimationTrackBase *);
  void update(const float time);

 private:
  std::vector<AnimationTrackBase *> tracks;
};

// an animation track, i.e. an array of keyframes = time:value pair
template <typename VALUE_T>
struct OSPSG_INTERFACE AnimationTrack : public AnimationTrackBase
{
  ~AnimationTrack() override = default;
  void update(const float time) override;

  std::vector<VALUE_T> values;
};

} // namespace sg
} // namespace ospray
