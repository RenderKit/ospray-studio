// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Animation.h"

namespace ospray {
namespace sg {

Animation::Animation(const std::string &name) : name(name) {}

void Animation::addTrack(AnimationTrackBase *track)
{
  if (track->times.size() == 0)
    return;

  tracks.push_back(track);
  timeRange.extend(*begin(track->times));
  timeRange.extend(track->times.back());
}

void Animation::update(const float time)
{
  if (!active)
    return;

  for (auto &t : tracks)
    t->update(time);
}

void AnimationTrackBase::updateIndex(const float time)
{
  // check if index still valid wrt. time
  if ((index < times.size() && times[index] <= time)
      && (index + 1 >= times.size() || times[index + 1] > time))
    return;
  index = distance(begin(times), lower_bound(begin(times), end(times), time));
}

template <typename VALUE_T>
void AnimationTrack<VALUE_T>::update(const float time)
{
  updateIndex(time);

  switch (interpolation) {
  case InterpolationMode::LINEAR:
  case InterpolationMode::CUBIC:
  case InterpolationMode::STEP: /* fallthrough, step is default */
  default:
    target->setValue(values[min(index, values.size() - 1)]);
  }
}

template void AnimationTrack<float>::update(const float time);
template void AnimationTrack<vec3f>::update(const float time);
template void AnimationTrack<quaternionf>::update(const float time);

} // namespace sg
} // namespace ospray
