// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <algorithm>
#include <stdexcept>
#include <utility>
#include <vector>

#include "ArcballCamera.h"

// A template wrapper around std::vector for implementing a generic CameraStack

template <typename VALUE>
struct CameraStack
{
  using storage_t = std::vector<VALUE>;
  using iterator_t = decltype(std::declval<storage_t>().begin());
  using citerator_t = decltype(std::declval<storage_t>().cbegin());
  using riterator_t = decltype(std::declval<storage_t>().rbegin());
  using criterator_t = decltype(std::declval<storage_t>().crbegin());

  CameraStack() = default;
  CameraStack(std::shared_ptr<ArcballCamera> _arcballCamera)
      : arcballCamera(_arcballCamera)
  {}
  ~CameraStack() = default;

  // Index-based lookups //

  VALUE &at(size_t index);

  VALUE &operator[](size_t index);

  // Property queries //

  size_t size() const;
  size_t empty() const;

  // Storage mutation //
  void clear();
  void reserve(size_t size);
  void insert(VALUE value);
  void push_back(VALUE value);
  VALUE& back();

  // Iterators //

  iterator_t begin();
  citerator_t begin() const;

  iterator_t end();
  citerator_t end() const;

  // class functions
  void addCameraState();
  void removeCameraState();
  bool setCameraSnapshot(size_t snapshot);
  bool selectCamStackIndex(size_t i);
  void pushLookMark();
  bool popLookMark();
  void updateArcball(std::shared_ptr<ArcballCamera> newArcballCamera)
  {
    arcballCamera = newArcballCamera;
  }

  void setValues(storage_t _values)
  {
    values = _values;
  }
  // build an interpolated path from a vector of CameraStates
  // using Catmull-Rom quaternion interpolation
  // for n >= 2 anchors, creates (n - 1) * (1 / stepSize) CameraStates
  CameraStack<VALUE> buildPath();

  static int g_camSelectedStackIndex;
  float g_camPathSpeed = 5 * 0.01f; // defined in hundredths (e.g. 10 = 10 * 0.01 = 0.1)
  int g_camCurrentPathIndex = 0;

  // Helper functions for building CameraStack paths
  CameraState slerp(
      const CameraState &from, const CameraState &to, float frac) const
  {
    CameraState cs;

    cs.centerTranslation = lerp(frac, from.centerTranslation, to.centerTranslation);
    cs.translation = lerp(frac, from.translation, to.translation);
    if (from.rotation != to.rotation)
      cs.rotation = ospray::sg::slerp(from.rotation, to.rotation, frac);
    else
      cs.rotation = from.rotation;

    return cs;
  }

  // Catmull-Rom interpolation for rotation quaternions
  // linear interpolation for translation matrices
  // adapted from Graphics Gems 2
  CameraState catmullRom(const CameraState &prefix,
      const CameraState &from,
      const CameraState &to,
      const CameraState &suffix,
      float frac)
  {
    if (frac == 0) {
      return from;
    } else if (frac == 1) {
      return to;
    }

    // essentially this interpolation creates a "pyramid"
    // interpolate 4 points to 3
    CameraState c10 = slerp(prefix, from, frac + 1);
    CameraState c11 = slerp(from, to, frac);
    CameraState c12 = slerp(to, suffix, frac - 1);

    // 3 points to 2
    CameraState c20 = slerp(c10, c11, (frac + 1) / 2.f);
    CameraState c21 = slerp(c11, c12, frac / 2.f);

    // and 2 to 1
    CameraState cf = slerp(c20, c21, frac);

    return cf;
  }

 private:
  // Data //
  storage_t values;
  std::shared_ptr<ArcballCamera> arcballCamera = nullptr;
};

// Inlined definitions ////////////////////////////////////////////////////

template <typename VALUE>
inline VALUE &CameraStack<VALUE>::at(size_t index)
{
  return values.at(index);
}

template <typename VALUE>
inline VALUE &CameraStack<VALUE>::operator[](size_t index)
{
  return values.at(index);
}

template <typename VALUE>
inline size_t CameraStack<VALUE>::size() const
{
  return values.size();
}

template <typename VALUE>
inline size_t CameraStack<VALUE>::empty() const
{
  return values.empty();
}

template <typename VALUE>
inline void CameraStack<VALUE>::clear()
{
  values.clear();
}

template <typename VALUE>
inline void CameraStack<VALUE>::reserve(size_t size)
{
  return values.reserve(size);
}

template <typename VALUE>
inline void CameraStack<VALUE>::insert(VALUE val)
{
  values.push_back(val);
}

template <typename VALUE>
inline void CameraStack<VALUE>::push_back(VALUE val)
{
  values.push_back(val);
}


template <typename VALUE>
inline VALUE &CameraStack<VALUE>::back()
{
  return values.back();
}
// Iterators //

template <typename VALUE>
inline typename CameraStack<VALUE>::iterator_t CameraStack<VALUE>::begin()
{
  return values.begin();
}

template <typename VALUE>
inline typename CameraStack<VALUE>::citerator_t CameraStack<VALUE>::begin()
    const
{
  return values.cbegin();
}

template <typename VALUE>
inline typename CameraStack<VALUE>::iterator_t CameraStack<VALUE>::end()
{
  return values.end();
}

template <typename VALUE>
inline typename CameraStack<VALUE>::citerator_t CameraStack<VALUE>::end() const
{
  return values.cend();
}

//initialize static member variables
template <typename VALUE>
int CameraStack<VALUE>::g_camSelectedStackIndex = 0;

template <typename VALUE>
void CameraStack<VALUE>::addCameraState()
{
  if (values.empty()) {
    values.push_back(arcballCamera->getState());
    g_camSelectedStackIndex = 0;
  } else {
    values.insert(values.begin() + g_camSelectedStackIndex + 1,
        arcballCamera->getState());
    g_camSelectedStackIndex++;
  }
}

template <typename VALUE>
void CameraStack<VALUE>::removeCameraState()
{
  // remove the selected camera state
  values.erase(values.begin() + g_camSelectedStackIndex);
  g_camSelectedStackIndex = std::max(0, g_camSelectedStackIndex - 1);
}

template <typename VALUE>
bool CameraStack<VALUE>::setCameraSnapshot(size_t snapshot)
{
  if (snapshot < values.size()) {
    const CameraState &cs = values.at(snapshot);
    arcballCamera->setState(cs);
    return true;
  }
  return false;
}

template <typename VALUE>
bool CameraStack<VALUE>::selectCamStackIndex(size_t i)
{
  g_camSelectedStackIndex = i;
  if (values.empty() || i >= values.size())
    return false;
  arcballCamera->setState(values.at(i));
  return true;
}

template <typename VALUE>
void CameraStack<VALUE>::pushLookMark()
{
  values.push_back(arcballCamera->getState());
  vec3f from = arcballCamera->eyePos();
  vec3f up = arcballCamera->upDir();
  vec3f at = arcballCamera->lookDir() + from;
  fprintf(stderr,
      "-vp %f %f %f -vu %f %f %f -vi %f %f %f\n",
      from.x,
      from.y,
      from.z,
      up.x,
      up.y,
      up.z,
      at.x,
      at.y,
      at.z);
}

template <typename VALUE>
bool CameraStack<VALUE>::popLookMark()
{
  if (values.empty())
    return false;
  CameraState cs = values.back();
  values.pop_back();

  arcballCamera->setState(cs);
  return true;
}

template <typename VALUE>
CameraStack<VALUE> CameraStack<VALUE>::buildPath()
{
  if (values.size() < 2) {
    std::cout << "Must have at least 2 values to create path!" << std::endl;
    return {};
  }

  // in order to touch all provided anchor, we need to extrapolate a new anchor
  // on both ends for Catmull-Rom's prefix/suffix
  size_t last = values.size() - 1;
  CameraState prefix = slerp(values[0], values[1], -0.1f);
  CameraState suffix = slerp(values[last - 1], values[last], 1.1f);

  CameraStack<CameraState> path;

  for (size_t i = 0; i < last; i++) {
    CameraState c0 = (i == 0) ? prefix : values[i - 1];
    CameraState c1 = values[i];
    CameraState c2 = values[i + 1];
    CameraState c3 = (i == (last - 1)) ? suffix : values[i + 2];

    for (float frac = 0.f; frac < 1.f; frac += g_camPathSpeed) {
      path.push_back(catmullRom(c0, c1, c2, c3, frac));
    }
  }

  return path;
}
