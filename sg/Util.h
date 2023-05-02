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

// linear_to_srgba8(const T c)
// srgb_to_linear(const T c)
// 2.23333f provides a much more accurate approximation than 2.2
static const float gamma_const = 2.23333f;
inline float _pow(const float base, const float exp)
{
  return std::pow(std::max(base, 0.f), exp);
};
inline vec3f _pow(const vec3f base3, const float exp)
{
  return vec3f(_pow(base3.x, exp), _pow(base3.y, exp), _pow(base3.z, exp));
};
inline vec4f _pow(const vec4f base4, const float exp)
{
  // alpha is never corrected
  vec3f g = _pow(rgb(base4.x, base4.y, base4.z), exp);
  return vec4f(g.x, g.y, g.z, base4.w);
};

template <typename T>
inline void srgb_to_linear(T &c)
{
  c = _pow(c, gamma_const);
}
template <typename T>
inline void linear_to_srgb(T &c)
{
  c = _pow(c, 1.f / gamma_const);
}

// Generate a "random" color given a 32-bit integer index
// Identical function to rkcommon/utility/random.h, however, there are conflicts
// within rkcommon/utility/detail/pcg_random.hpp.  So, duplicating here.
inline vec4f makeRandomColor(const int i)
{
  const int mx = 13 * 17 * 43;
  const int my = 11 * 29;
  const int mz = 7 * 23 * 63;
  const uint32_t g = (i * (3 * 5 * 127) + 12312314);
  return vec4f((g % mx) * (1.f / (mx - 1)),
      (g % my) * (1.f / (my - 1)),
      (g % mz) * (1.f / (mz - 1)),
      1.0f);
}

} // namespace sg
} // namespace ospray
