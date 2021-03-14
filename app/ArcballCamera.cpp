// Copyright 2017-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "ArcballCamera.h"

ArcballCamera::ArcballCamera(const box3f &worldBounds, const vec2i &windowSize)
    : zoomSpeed(1),
      invWindowSize(vec2f(1.0) / vec2f(windowSize)),
      centerTranslation(one),
      translation(one),
      rotation(one)
{
  float diag = length(worldBounds.size());

  zoomSpeed  = max(diag / 150.0, 0.001);

  // if Box3f defining wolrd bounds is less than a unit cube
  // translate along (0, 0, 1)
  if (diag < 1.7)
    diag = 1.7f;;

  centerTranslation = AffineSpace3f::translate(-worldBounds.center());
  translation       = AffineSpace3f::translate(vec3f(0, 0, -diag));
  updateCamera();
}

void ArcballCamera::setNewWorldBounds(const box3f &worldBounds) {
  centerTranslation = AffineSpace3f::translate(-worldBounds.center());
  updateCamera();
}

void ArcballCamera::rotate(const vec2f &from, const vec2f &to)
{
  rotation = screenToArcball(to) * screenToArcball(from) * rotation;
  updateCamera();
}

void ArcballCamera::constrainedRotate(const vec2f &from, const vec2f &to, int axis)
{
  quaternionf nrot = screenToArcball(to) * screenToArcball(from);
  switch (axis) {
  case 0:
      nrot.j = nrot.k = 0;
      break;
  case 1:
      nrot.k = nrot.i = 0;
      break;
  case 2:
      nrot.i = nrot.j = 0;
      break;
  default:
      break;
  }
  nrot = normalize(nrot);
  rotation = nrot * rotation;
  updateCamera();
}

void ArcballCamera::zoom(float amount)
{
  amount *= zoomSpeed;
  translation = AffineSpace3f::translate(vec3f(0, 0, -amount)) * translation;
  translation.p.z = std::min<float>(-zoomSpeed, translation.p.z);
  updateCamera();
}

void ArcballCamera::dolly(float amount)
{
  auto worldt = lookDir() * amount * zoomSpeed;
  centerTranslation = AffineSpace3f::translate(worldt) * centerTranslation;
  updateCamera();
}

void ArcballCamera::pan(const vec2f &delta)
{
  // XXX This should really be called "truck/pedestal". "pan/tilt" are
  // a fixed-base rotation about the camera (more like our rotate)
  const vec3f t =
      vec3f(delta.x * invWindowSize.x, delta.y * invWindowSize.y, 0);
  const vec3f worldt = abs(translation.p.z) * xfmVector(cameraToWorld, t);
  centerTranslation  = AffineSpace3f::translate(worldt) * centerTranslation;
  updateCamera();
}

vec3f ArcballCamera::eyePos() const
{
  return xfmPoint(cameraToWorld, vec3f(0, 0, 0));
}

void ArcballCamera::setCenter(const vec3f &newCenter)
{
  centerTranslation = AffineSpace3f::translate(-newCenter);
  updateCamera();
}

vec3f ArcballCamera::center() const
{
  return -centerTranslation.p;
}

vec3f ArcballCamera::lookDir() const
{
  return xfmVector(cameraToWorld, vec3f(0, 0, -1));
}

vec3f ArcballCamera::upDir() const
{
  return xfmVector(cameraToWorld, vec3f(0, 1, 0));
}

void ArcballCamera::updateCamera()
{
  const AffineSpace3f rot           = LinearSpace3f(rotation);
  const AffineSpace3f worldToCamera = translation * rot * centerTranslation;
  cameraToWorld                     = rcp(worldToCamera);
}

void ArcballCamera::setRotation(quaternionf q)
{
  rotation = q;
  updateCamera();
}

void ArcballCamera::setState(const CameraState &state)
{
  if (state.useCameraToWorld) {
    cameraToWorld = state.cameraToWorld;
  } else {
    centerTranslation = state.centerTranslation;
    translation = state.translation;
    rotation = state.rotation;
    updateCamera();
  }
}

void ArcballCamera::setZoomSpeed(float speed)
{
  zoomSpeed = speed;
}

CameraState ArcballCamera::getState() const
{
  return CameraState(centerTranslation, translation, rotation);
}

void ArcballCamera::updateWindowSize(const vec2i &windowSize)
{
  invWindowSize = vec2f(1) / vec2f(windowSize);
}

quaternionf ArcballCamera::screenToArcball(const vec2f &p)
{
  const float dist = dot(p, p);
  // If we're on/in the sphere return the point on it
  if (dist <= 1.f) {
    return quaternionf(0, p.x, -p.y, std::sqrt(1.f - dist));
  } else {
    // otherwise we project the point onto the sphere
    const vec2f unitDir = normalize(p);
    return quaternionf(0, unitDir.x, -unitDir.y, 0);
  }
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
  CameraState c10 = prefix.slerp(from, frac + 1);
  CameraState c11 = from.slerp(to, frac);
  CameraState c12 = to.slerp(suffix, frac - 1);

  // 3 points to 2
  CameraState c20 = c10.slerp(c11, (frac + 1) / 2.f);
  CameraState c21 = c11.slerp(c12, frac / 2.f);

  // and 2 to 1
  CameraState cf = c20.slerp(c21, frac);

  return cf;
}

std::vector<CameraState> buildPath(const std::vector<CameraState> &anchors,
    const float stepSize)
{
  if (anchors.size() < 2) {
    std::cout << "Must have at least 2 anchors to create path!" << std::endl;
    return {};
  }

  // in order to touch all provided anchor, we need to extrapolate a new anchor
  // on both ends for Catmull-Rom's prefix/suffix
  size_t last = anchors.size() - 1;
  CameraState prefix = anchors[0].slerp(anchors[1], -0.1f);
  CameraState suffix = anchors[last - 1].slerp(anchors[last], 1.1f);

  std::vector<CameraState> path;
  for (size_t i = 0; i < last; i++) {
    CameraState c0 = (i == 0) ? prefix : anchors[i - 1];
    CameraState c1 = anchors[i];
    CameraState c2 = anchors[i + 1];
    CameraState c3 = (i == (last - 1)) ? suffix : anchors[i + 2];

    for (float frac = 0.f; frac < 1.f; frac += stepSize) {
      path.push_back(catmullRom(c0, c1, c2, c3, frac));
    }
  }

  return path;
}
