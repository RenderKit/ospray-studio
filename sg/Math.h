// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "Node.h"

namespace ospray {
namespace sg {

// input fov: in degrees
// input resolution: final resolution of the image
// output 4x4 projection matrix
// (can also be called the K-matrix)
inline std::vector<std::vector<float>> perspectiveMatrix(
    float &fovy, vec2i &resolution)
{
  // scale by inverse FOV resolution
  // flip Y axis because OSPRay has image start as lower left corner
  // change w and z to -1 (see:
  // https://www.khronos.org/registry/glTF/specs/2.0/glTF-2.0.html#projection-matrices)
  auto x = 1.f / (2.f * tanf(deg2rad(0.5f * fovy)) * 1.5);
  auto y = 1.f / (2.f * tanf(deg2rad(0.5f * fovy)));
  float m[4][4];
  m[0][0] = x;
  m[0][1] = 0;
  m[0][2] = 0;
  m[0][3] = 0;
  m[1][0] = 0;
  m[1][1] = -y;
  m[1][2] = 0;
  m[1][3] = 0;
  m[2][0] = 0;
  m[2][1] = 0;
  m[2][2] = -1;
  m[2][3] = 0;
  m[3][0] = 0;
  m[3][1] = 0;
  m[3][2] = -1;
  m[3][3] = 0;

  // scale by final resolution and translate origin to center of image
  auto xscale = resolution[0];
  auto yscale = resolution[1];
  auto xorigin = xscale / 2.f;
  auto yorigin = yscale / 2.f;
  float mRes[4][4];
  mRes[0][0] = xscale;
  mRes[0][1] = 0;
  mRes[0][2] = xorigin;
  mRes[0][3] = 0;
  mRes[1][0] = 0;
  mRes[1][1] = yscale;
  mRes[1][2] = yorigin;
  mRes[1][3] = 0;
  mRes[2][0] = 0;
  mRes[2][1] = 0;
  mRes[2][2] = 1;
  mRes[2][3] = 0;
  mRes[3][0] = 0;
  mRes[3][1] = 0;
  mRes[3][2] = 0;
  mRes[3][3] = 1;

  std::vector<std::vector<float>> projMat;
  float sum;
  for (int i = 0; i < 4; i++) {
    std::vector<float> row(4);
    for (int j = 0; j < 4; j++) {
      sum = 0;
      for (int k = 0; k < 4; k++) {
        sum = sum + (mRes[i][k] * m[k][j]);
      }
      row[j] = sum;
    }
    projMat.push_back(row);
  }
#if 0  
  std::cout << "\nFinal projection matrix:\n";
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++)
      std::cout << projMat[i][j] << "\t";
    std::cout << std::endl;
  }
  std::cout << std::endl;
#endif
  return projMat;
}

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
