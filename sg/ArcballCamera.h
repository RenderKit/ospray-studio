// Copyright 2017 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <vector>

#include "rkcommon/math/AffineSpace.h"
#include "sg/Math.h"

#include "Node.h" // for OSPSG_INTERFACE

struct OSPSG_INTERFACE CameraState
{
 public:

  AffineSpace3f centerTranslation;
  AffineSpace3f translation;
  AffineSpace3f cameraToWorld;
  quaternionf rotation;
  bool useCameraToWorld{false};

  // helper functions
  vec3f position() const
  {
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
};

class OSPSG_INTERFACE ArcballCamera
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
    return cs.cameraToWorld;
  }

 protected:
  void updateCamera();

  // Project the point in [-1, 1] screen space onto the arcball sphere
  quaternionf screenToArcball(const vec2f &p);

  float worldDiag;  // length of the world bounds diagonal
  vec2f invWindowSize;
  CameraState cs;

  bool lockUpDir{false};
  vec3f upVec{0, 1, 0};
};

inline std::ostream &operator<<(std::ostream &os, const CameraState &cs)
{
  std::cout << "centerTranslation = " << cs.centerTranslation << ", ";
  std::cout << "translation = " << cs.translation << ", ";
  std::cout << "rotation = " << cs.rotation << std::endl;
  return os;
}
