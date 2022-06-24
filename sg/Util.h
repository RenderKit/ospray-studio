// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <regex>
#include "Node.h"
using namespace std;
using namespace std::regex_constants;

namespace ospray {
namespace sg {

// check 128-bit UUID for Version 4 format
inline bool isValidUUIDv4(const string &str)
{
  static const regex uuidv4Format(
      "^[[:xdigit:]]{8}-[[:xdigit:]]{4}-4[[:xdigit:]]{3}-[89ABab][[:xdigit:]]{3}-[[:xdigit:]]{12}$",
      nosubs | optimize);
  return regex_match(str, uuidv4Format);
}

inline unsigned int reduce24(unsigned int val)
{
  val &= 0xffffff; // save only first 24-bits
  return val;
}

// generates 24-bit hash for string of any length stored in 32-bit integer
// uses Robert Sedgewick algo for generating the hash
inline unsigned int getRSHash24(const std::string &str)
{
  unsigned int b = 378551;
  unsigned int a = 63689;
  unsigned int hash = 0;
  for (std::size_t i = 0; i < str.length(); i++) {
    hash = hash * a + str[i];
    a *= b;
  }
  hash &= 0xffffff; // save only first 24-bits
  return hash;
}

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

} // namespace sg
} // namespace ospray
