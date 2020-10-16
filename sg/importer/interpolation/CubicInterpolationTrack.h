// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <vector>

#include "rkcommon/math/AffineSpace.h"

using namespace rkcommon::math;

namespace ospray {
namespace sg {

inline std::pair<float, affine3f> catmullRomInterpolant(
    std::pair<float, affine3f> &prefix,
    std::pair<float, affine3f> &from,
    std::pair<float, affine3f> &to,
    std::pair<float, affine3f> &suffix,
    float currentStep)
{
  if (currentStep == 0)
    return from;
  else if (currentStep == 1)
    return to;

  auto k10 = kfLerp(prefix, from, currentStep + 1);
  auto k11 = kfLerp(from, to, currentStep);
  auto k12 = kfLerp(to, suffix, currentStep - 1);

  auto k20 = kfLerp(k10, k11, (currentStep + 1) / 2.f);
  auto k21 = kfLerp(k11, k12, currentStep / 2.f);

  auto newKeyframe = kfLerp(k20, k21, currentStep);
  return newKeyframe;
}

inline void crInterpolationTSTrack(std::vector<float> &kfInput,
    std::vector<affine3f> &kfOutput,
    float stepIncrement,
    std::map<float, affine3f> &keyframeTrack)
{
  auto last = kfInput.size() - 1;

  auto prefix = kfLerp(std::make_pair(kfInput[0], kfOutput[0]),
      std::make_pair(kfInput[1], kfOutput[1]),
      -0.1f);
  auto suffix = kfLerp(std::make_pair(kfInput[last - 1], kfOutput[last - 1]),
      std::make_pair(kfInput[last], kfOutput[last]),
      1.1f);

  for (auto i = 0; i < last; i++) {
    auto k0 =
        (i == 0) ? prefix : std::make_pair(kfInput[i - 1], kfOutput[i - 1]);
    auto k1 = std::make_pair(kfInput[i], kfOutput[i]);
    auto k2 = std::make_pair(kfInput[i + 1], kfOutput[i + 1]);
    auto k3 = (i == (last - 1))
        ? suffix
        : std::make_pair(kfInput[i + 2], kfOutput[i + 2]);
    for (float step = 0.f; step <= 1.f; step += stepIncrement) {
      keyframeTrack.insert(catmullRomInterpolant(k0, k1, k2, k3, step));
    }
  }
}

inline float cube(float &t) {
  return t * t  * t;
}
inline float square(float &t) {
  return t * t;
}

template<typename T>
inline T hermiteInterpolant(
    T &startPoint,
    T &startTangent,
    T &endPoint,
    T &endTangent,
    float &t)
{
    // return resulting point value p(t) at time t
    //  p(t) = (2t^3 - 3t^2 + 1)p0 + (t^3 - 2t^2 + t)m0 + (-2t^3 + 3t^2)p1 + (t^3 - t^2)m1

    auto p0 = (2 * cube(t) - 3 * square(t) + 1) * startPoint;
    auto m0 = (cube(t) - 2 * square(t) + t) * startTangent;
    auto p1 = ((-2) * cube(t) + 3 * square(t)) * endPoint;
    auto m1 = (cube(t) - square(t)) * endTangent;
    return p0 + m0 + p1 + m1;
}

// we assume data in output is laid out like : in-tangent, point, out-tangent

// Implementation Note: When writing out rotation output values,
// do not write out values which can result in an invalid
// quaternion with all zero values. This can be achieved by ensuring the output
// values never have both -q and q in the same spline.

// Implementation Note: The first in-tangent a1 and last out-tangent bn should
// be zeros as they are not used in the spline calculations.
inline void hermiteInterpolationTSTrack(std::vector<float> &kfInput,
    std::vector<vec3f> &ts,
    float stepIncrement,
    std::map<float, affine3f> &keyframeTrack,
    std::string &targetPath)
{
  for (auto i = 0; i < kfInput.size() - 1; ++i) {
    // for every input find t, startPoint, startTangent, endPoint, endTangent
    auto index_from = i;
    auto index_to = i + 1;

    auto time_from = kfInput[index_from];
    auto time_to = kfInput[index_to];

    // output from
    auto ak = ts[index_from * 3];
    auto vk = ts[index_from * 3 + 1];
    auto bk = ts[index_from * 3 + 2];

    // output to
    auto akNext = ts[index_to * 3];
    auto vkNext = ts[index_to * 3 + 1];
    auto bkNext = ts[index_to * 3 + 2];

    auto tDelta = time_to - time_from;

    // now if we want to calculate interpolated times at interval of
    // stepIncrement:
    for (float step = 0.f; step <= 1.f; step += stepIncrement) {
      auto tcurrent = rkcommon::math::lerp(step, time_from, time_to);
      auto t = (tcurrent - time_from) / tDelta;
      auto p0 = vk;
      auto m0 = tDelta * bk;
      auto p1 = vkNext;
      auto m1 = tDelta * akNext;
      // Implementation Note: The first in-tangent ak and last out-tangent
      // bkNext should be zeros as they are not used in the spline calculations.
      auto result = hermiteInterpolant<vec3f>(p0, m0, p1, m1, t);
      affine3f xfm{one};
      std::pair<float, affine3f> newKeyframe;
      newKeyframe.first = tcurrent;
      if (targetPath == "translation")
        xfm = affine3f::translate(vec3f(result[0], result[1], result[2])) * xfm;
      else
        xfm = affine3f::scale(vec3f(result[0], result[1], result[2])) * xfm;
      newKeyframe.second = xfm;
      keyframeTrack.insert(newKeyframe);
    }
  }
}

inline void hermiteInterpolationRotationtrack(std::vector<float> &kfInput,
    std::vector<vec4f> &rotations,
    float stepIncrement,
    std::map<float, affine3f> &keyframeTrack)
{

    for (auto i = 0; i < kfInput.size() - 1; ++i) {
    // for every input find t, startPoint, startTangent, endPoint, endTangent
    auto index_from = i;
    auto index_to = i + 1;

    auto time_from = kfInput[index_from];
    auto time_to = kfInput[index_to];

    // output from
    auto ak = rotations[index_from * 4];
    auto vk = rotations[index_from * 4 + 1];
    auto bk = rotations[index_from * 4 + 2];

    // output to
    auto akNext = rotations[index_to * 4];
    auto vkNext = rotations[index_to * 4 + 1];
    auto bkNext = rotations[index_to * 4 + 2];

    auto tDelta = time_to - time_from;

    // now if we want to calculate interpolated times at interval of
    // stepIncrement:
    for (float step = 0.f; step <= 1.f; step += stepIncrement) {
      auto tcurrent = rkcommon::math::lerp(step, time_from, time_to);
      auto t = (tcurrent - time_from) / tDelta;
      auto p0 = vk;
      auto m0 = tDelta * bk;
      auto p1 = vkNext;
      auto m1 = tDelta * akNext;
      // Implementation Note: The first in-tangent ak and last out-tangent
      // bkNext should be zeros as they are not used in the spline calculations.
      auto r = hermiteInterpolant<vec4f>(p0, m0, p1, m1, t);
      quaternionf rot;
      if (r != vec4f(0.f))
        rot = normalize(quaternionf(r[3], r[0], r[1], r[2]));
      else
        rot = quaternionf(r[3], r[0], r[1], r[2]);
      affine3f xfm{one};
      std::pair<float, affine3f> newKeyframe;
      newKeyframe.first = tcurrent;
      xfm = affine3f(linear3f(rot)) * xfm;
      newKeyframe.second = xfm;
      keyframeTrack.insert(newKeyframe);
    }
  }
}

} // namespace sg
} // namespace ospray
