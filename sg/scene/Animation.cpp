// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Animation.h"

namespace ospray {
namespace sg {

Animation::Animation(const std::string &name) : name(name) {}

void Animation::addTrack(AnimationTrackBase *track)
{
  if (!track->valid())
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
  if ((index < 0 || times[index] <= time)
      && (index + 1 >= (ssize_t) times.size() || time < times[index + 1]))
    return;
  const auto &start = begin(times);
  index = distance(start, upper_bound(start, end(times), time)) - 1;
}

template <typename VALUE_T>
bool AnimationTrack<VALUE_T>::valid()
{
  auto s = times.size();
  if (interpolation == InterpolationMode::CUBIC)
    s *= 3;
  return s > 0 && s == values.size();
}

template <typename T>
inline T lerp(const float factor, const T &a, const T &b)
{
  return rkcommon::math::lerp(factor, a, b);
}

template <>
inline quaternionf lerp<quaternionf>(
    const float factor, const quaternionf &a, const quaternionf &b)
{
  return rkcommon::math::slerp(factor, a, b);
}

// quaternions need normalization
template <typename T>
inline T quatnorm(const T &q)
{
  return q; // noop
}

template <>
inline quaternionf quatnorm(const quaternionf &q)
{
  return rkcommon::math::normalize(q); // noop
}

// p(t) =
//   (2t^3 - 3t^2 + 1)p0 + (t^3 - 2t^2 + t)m0 + (-2t^3 + 3t^2)p1 + (t^3 - t^2)m1
template <typename T>
inline T cspline(
    const float t, const T &p0, const T &m0, const T &p1, const T &m1)
{
  const float t2 = t * t;
  const float t3 = t2 * t;
  const float c0 = 2.f * t3 - 3.f * t2 + 1.f;
  const float c1 = t3 - 2.f * t2 + t;
  const float c2 = -2.f * t3 + 3.f * t2;
  const float c3 = t3 - t;
  const T p = c0 * p0 + c1 * m0 + c2 * p1 + c3 * m1;
  return sg::quatnorm(p);
}

template <typename VALUE_T>
void AnimationTrack<VALUE_T>::update(const float time)
{
  updateIndex(time);
  const ssize_t idx0 = std::max(index, ssize_t(0));
  const bool isCubic = interpolation == InterpolationMode::CUBIC;
  auto val = values[isCubic ? idx0 * 3 + 1 : idx0];

  if (isCubic || interpolation == InterpolationMode::LINEAR) {
    size_t idx1 = std::min(size_t(index + 1), times.size() - 1);
    const float time0 = times[idx0];
    const float difft = times[idx1] - time0;
    if (difft > 0.0f) {
      const auto val1 = values[isCubic ? idx1 * 3 + 1 : idx1];
      const float t = (time - time0) / difft;
      if (isCubic) {
        val = sg::cspline(t,
            val,
            difft * values[idx0 * 3 + 2],
            val1,
            difft * values[idx1 * 3]);
      } else
        val = sg::lerp(t, val, val1);
    }
  }

  target->setValue(val);
}

template <>
void AnimationTrack<NodePtr>::update(const float time)
{
  updateIndex(time);
  target->add(values[std::max(index, ssize_t(0))], "timeseries");
}

template void AnimationTrack<float>::update(const float);
template bool AnimationTrack<float>::valid();
template void AnimationTrack<vec3f>::update(const float);
template bool AnimationTrack<vec3f>::valid();
template void AnimationTrack<quaternionf>::update(const float);
template bool AnimationTrack<quaternionf>::valid();
template void AnimationTrack<NodePtr>::update(const float);
template bool AnimationTrack<NodePtr>::valid();

} // namespace sg
} // namespace ospray
