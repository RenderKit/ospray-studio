// Copyright 2017-2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <vector>

#include "rkcommon/math/AffineSpace.h"

using namespace rkcommon::math;

class ArcballCamera;

class CameraState
{
 public:
  CameraState() = default;
  CameraState(const AffineSpace3f &centerTrans,
      const AffineSpace3f &trans,
      const quaternionf &rot,
      const AffineSpace3f &camToWorld)
      : centerTranslation(centerTrans),
        translation(trans),
        rotation(rot),
        cameraToWorld(camToWorld)
  {}

  CameraState slerp(const CameraState &to, float frac) const
  {
    CameraState cs;

    cs.centerTranslation = lerp(frac, centerTranslation, to.centerTranslation);
    cs.translation       = lerp(frac, translation, to.translation);
    if (rotation != to.rotation)
      cs.rotation = slerp(rotation, to.rotation, frac);
    else
      cs.rotation = rotation;

    return cs;
  }

  vec3f position() const {
    const AffineSpace3f rot = LinearSpace3f(rotation);
    const AffineSpace3f camera = translation * rot * centerTranslation;
    return xfmPoint(rcp(camera), vec3f(0, 0, 1));
  }

  friend std::string to_string(const CameraState &state)
  {
    // returns the world position
    vec3f pos = state.position();
    std::stringstream ss;
    ss << pos;
    return ss.str();
  }

  // participate in Cerealization
  template<class Archive>
  void serialize(Archive & archive)
  {
    archive(centerTranslation, translation, rotation, cameraToWorld);
  }

  AffineSpace3f centerTranslation, translation;
  quaternionf rotation;

  AffineSpace3f cameraToWorld;
  bool useCameraToWorld{false};

 protected:
  friend ArcballCamera;

  float dot(const quaternionf &q0, const quaternionf &q1) const
  {
    return q0.r * q1.r + q0.i * q1.i + q0.j * q1.j + q0.k * q1.k;
  }

  quaternionf slerp(const quaternionf &q0, const quaternionf &q1, float t) const
  {
    quaternionf qt0 = q0, qt1 = q1;
    float d = dot(qt0, qt1);
    if (d < 0.f) {
      // prevent "long way around"
      qt0 = -qt0;
      d   = -d;
    } else if (d > 0.9995) {
      // angles too small
      quaternionf l = lerp(t, q0, q1);
      return normalize(l);
    }

    float theta  = std::acos(d);
    float thetat = theta * t;

    float s0 = std::cos(thetat) - d * std::sin(thetat) / std::sin(theta);
    float s1 = std::sin(thetat) / std::sin(theta);

    return s0 * qt0 + s1 * qt1;
  }
};

class ArcballCamera
{
 public:
  ArcballCamera(const box3f &worldBounds, const vec2i &windowSize);

  void updateCameraToWorld(
      const affine3f &cameraToWorld, const quaternionf &rot);

  // All mouse positions passed should be in [-1, 1] normalized screen coords
  void rotate(const vec2f &from, const vec2f &to);
  void constrainedRotate(const vec2f &from, const vec2f &to, int axis /* 0 = x, 1 = y, 2 = z, otherwise none*/);
  void zoom(float amount);
  void dolly(float amount);
  void pan(const vec2f &delta);

  vec3f eyePos() const;
  vec3f center() const;
  void setCenter(const vec3f &newCenter);

  vec3f lookDir() const;
  vec3f upDir() const;

  float getZoomLevel();
  void setZoomLevel(float zoomLevel);

  void setRotation(quaternionf);
  void setState(const CameraState &state);
  CameraState getState() const;

  void updateWindowSize(const vec2i &windowSize);
  void setNewWorldBounds(const box3f &worldBounds);

  void setLockUpDir(bool locked);
  void setUpDir(vec3f newDir);

  affine3f getTransform()
  {
    return cameraToWorld;
  }

 protected:
  void updateCamera();

  // Project the point in [-1, 1] screen space onto the arcball sphere
  quaternionf screenToArcball(const vec2f &p);

  float worldDiag;  // length of the world bounds diagonal
  vec2f invWindowSize;
  AffineSpace3f centerTranslation, translation, cameraToWorld;
  quaternionf rotation;

  bool lockUpDir{false};
  vec3f upVec{0, 1, 0};
};

// Catmull-Rom quaternion interpolation
// requires "endpoint" states `prefix` and `suffix`
// returns an interpolated state at `frac` [0, 1] between `from` and `to`
CameraState catmullRom(const CameraState &prefix,
                       const CameraState &from,
                       const CameraState &to,
                       const CameraState &suffix,
                       float frac);

// build an interpolated path from a vector of CameraStates
// using Catmull-Rom quaternion interpolation
// for n >= 2 anchors, creates (n - 1) * (1 / stepSize) CameraStates
std::vector<CameraState> buildPath(const std::vector<CameraState> &anchors,
                                   const float stepSize = 0.1);

inline std::ostream &operator<<(std::ostream &os, const CameraState &cs)
{
  std::cout << "centerTranslation = " << cs.centerTranslation << ", ";
  std::cout << "translation = " << cs.translation << ", ";
  std::cout << "rotation = " << cs.rotation << std::endl;
  return os;
}
