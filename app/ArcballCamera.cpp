// Copyright 2017-2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "ArcballCamera.h"

ArcballCamera::ArcballCamera(const box3f &worldBounds, const vec2i &windowSize)
    : worldDiag(1),
      invWindowSize(vec2f(1.0) / vec2f(windowSize)),
      centerTranslation(one),
      translation(one),
      rotation(one)
{
  // Affects camera placement and camera movement.
  worldDiag = length(worldBounds.size());

  centerTranslation = AffineSpace3f::translate(-worldBounds.center());
  translation       = AffineSpace3f::translate(vec3f(0, 0, -worldDiag));
  updateCamera();
}

void ArcballCamera::updateCameraToWorld(
    const affine3f &_cameraToWorld, const quaternionf &rot)
{
  cameraToWorld = _cameraToWorld;
  auto worldToCamera = rcp(cameraToWorld);
  // update Translation and Rotation matrices
  affine3f newTrans{one};
  newTrans.p = worldToCamera.p;
  translation = newTrans;
  rotation = rot;
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
  amount *= worldDiag;
  translation = AffineSpace3f::translate(vec3f(0, 0, -amount)) * translation;
  // Don't allow zooming through the center of the arcBall
  translation.p.z = std::min<float>(0.f, translation.p.z);
  updateCamera();
}

void ArcballCamera::dolly(float amount)
{
  auto worldt = lookDir() * amount * worldDiag;
  centerTranslation = AffineSpace3f::translate(worldt) * centerTranslation;
  updateCamera();
}

void ArcballCamera::pan(const vec2f &delta)
{
  // XXX This should really be called "truck/pedestal". "pan/tilt" are
  // a fixed-base rotation about the camera (more like our rotate)
  const vec3f t = vec3f(delta.x, -delta.y, 0.f) * worldDiag;
  const vec3f worldt = 0.1f * xfmVector(cameraToWorld, t);
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
  return lockUpDir ? upVec : xfmVector(cameraToWorld, vec3f(0, 1, 0));
}

void ArcballCamera::setLockUpDir(bool locked)
{
  lockUpDir = locked;
}

void ArcballCamera::setUpDir(vec3f newDir)
{
  upVec = newDir;
}

void ArcballCamera::updateCamera()
{
  const AffineSpace3f rot           = LinearSpace3f(rotation);
  const AffineSpace3f worldToCamera = translation * rot * centerTranslation;
  cameraToWorld = rcp(worldToCamera);
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

float ArcballCamera::getZoomLevel()
{
  return translation.p.z;
}

void ArcballCamera::setZoomLevel(float zoomLevel)
{
  translation.p.z = zoomLevel;
  updateCamera();
}

CameraState ArcballCamera::getState() const
{
  return CameraState(centerTranslation, translation, rotation, cameraToWorld);
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
