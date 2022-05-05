// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "Node.h"
using namespace std;

namespace ospray {
namespace sg {

// convert euler angles(in radians) to quaternion
inline quaternionf eulerToQuaternion(const vec3f &rotation)
{
  float cy = cos(rotation[2] * 0.5);
  float sy = sin(rotation[2] * 0.5);
  float cp = cos(rotation[1] * 0.5);
  float sp = sin(rotation[1] * 0.5);
  float cr = cos(rotation[0] * 0.5);
  float sr = sin(rotation[0] * 0.5);

  auto w = cr * cp * cy + sr * sp * sy;
  auto x = sr * cp * cy - cr * sp * sy;
  auto y = cr * sp * cy + sr * cp * sy;
  auto z = cr * cp * sy - sr * sp * cy;

  return quaternionf(w, x, y, z);
}

inline quaternionf getRotationQuaternion(const LinearSpace3f &rot)
{
  float t = rot.vx[0] + rot.vy[1] + rot.vz[2];
  float w, x, y, z;
  if (t > 0) {
    float s = 0.5f / std::sqrt(t + 1.0f);
    w = 0.25f / s;
    x = (rot.vy[2] - rot.vz[1]) * s;
    y = (rot.vz[0] - rot.vx[2]) * s;
    z = (rot.vx[1] - rot.vy[0]) * s;
  } else {
    if (rot.vx[0] > rot.vy[1] && rot.vx[0] > rot.vz[2]) {
      float s = 2 * std::sqrt(1.0f + rot.vx[0] - rot.vy[1] - rot.vz[2]);
      w = (rot.vy[2] - rot.vz[1]) / s;
      x = 0.25f * s;
      y = (rot.vy[0] + rot.vx[1]) / s;
      z = (rot.vz[0] + rot.vx[2]) / s;
    } else if (rot.vy[1] > rot.vz[2]) {
      float s = 2.0f * sqrtf(1.0f + rot.vy[1] - rot.vx[0] - rot.vz[2]);
      w = (rot.vz[0] - rot.vx[2]) / s;
      x = (rot.vy[0] + rot.vx[1]) / s;
      y = 0.25f * s;
      z = (rot.vz[1] + rot.vy[2]) / s;
    } else {
      float s = 2.0f * sqrtf(1.0f + rot.vz[2] - rot.vx[0] - rot.vy[1]);
      w = (rot.vx[1] - rot.vy[0]) / s;
      x = (rot.vz[0] + rot.vx[2]) / s;
      y = (rot.vz[1] + rot.vy[2]) / s;
      z = 0.25f * s;
    }
  }
  return quaternionf(w, x, y, z);
}

inline LinearSpace3f getRotationMatrix(const LinearSpace3f &xfmM) {
  // Polar decomposition of matrix
  int count(0);
  float norm(0.f);
  LinearSpace3f rot = xfmM;
  do {
    // compute next matrix rotNext in series
    LinearSpace3f rotNext;
    auto rotInvTranspose = rot.transposed().inverse();

    for (int i = 0; i < 3; ++i) {
      rotNext.vx[i] = 0.5f * (rot.vx[i] + rotInvTranspose.vx[i]);
    }
    for (int i = 0; i < 3; ++i) {
      rotNext.vy[i] = 0.5f * (rot.vy[i] + rotInvTranspose.vy[i]);
    }
    for (int i = 0; i < 3; ++i) {
      rotNext.vz[i] = 0.5f * (rot.vz[i] + rotInvTranspose.vz[i]);
    }

    // norm of difference between rot and rotNext
    float n;
    auto diffX = rot.vx - rotNext.vx;
    n = std::abs(diffX[0] + diffX[1] + diffX[2]);
    norm = std::max(norm, n);
    auto diffY = rot.vy - rotNext.vy;
    n = std::abs(diffY[0] + diffY[1] + diffY[2]);
    norm = std::max(norm, n);
    auto diffZ = rot.vz - rotNext.vz;
    n = std::abs(diffZ[0] + diffZ[1] + diffZ[2]);
    norm = std::max(norm, n);

    rot = rotNext;

  } while (++count < 100 && norm > 0.001);
  return rot;
}

inline void getRSComponent(const affine3f &xfm, LinearSpace3f &R, LinearSpace3f &S)
{
  R = getRotationMatrix(xfm.l);
  S = R.inverse() * xfm.l;
}

} // namespace sg
} // namespace ospray
