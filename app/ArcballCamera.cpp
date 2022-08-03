// Copyright 2017 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "ArcballCamera.h"

ArcballCamera::ArcballCamera(const box3f &worldBounds, const vec2i &windowSize)
    : worldDiag(1),
      invWindowSize(vec2f(1.0) / vec2f(windowSize))
{
  cs.centerTranslation = one;
  cs.translation = one;
  cs.rotation = one;
  // Affects camera placement and camera movement.
  worldDiag = length(worldBounds.size());

  cs.centerTranslation = AffineSpace3f::translate(-worldBounds.center());
  cs.translation       = AffineSpace3f::translate(vec3f(0, 0, -worldDiag));
  updateCamera();
}

void ArcballCamera::updateCameraToWorld(
    const affine3f &_cameraToWorld, const quaternionf &rot)
{
  cs.cameraToWorld = _cameraToWorld;
  cs.rotation = rot;
}

void ArcballCamera::setNewWorldBounds(const box3f &worldBounds) {
  cs.centerTranslation = AffineSpace3f::translate(-worldBounds.center());
  updateCamera();
}

void ArcballCamera::rotate(const vec2f &from, const vec2f &to)
{
  cs.rotation = screenToArcball(to) * screenToArcball(from) * cs.rotation;
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
  cs.rotation = nrot * cs.rotation;
  updateCamera();
}

void ArcballCamera::zoom(float amount)
{
  amount *= worldDiag;
  cs.translation = AffineSpace3f::translate(vec3f(0, 0, -amount)) * cs.translation;
  // Don't allow zooming through the center of the arcBall
  cs.translation.p.z = std::min<float>(0.f, cs.translation.p.z);
  updateCamera();
}

void ArcballCamera::dolly(float amount)
{
  auto worldt = lookDir() * amount * worldDiag;
  cs.centerTranslation = AffineSpace3f::translate(worldt) * cs.centerTranslation;
  updateCamera();
}

void ArcballCamera::pan(const vec2f &delta)
{
  // XXX This should really be called "truck/pedestal". "pan/tilt" are
  // a fixed-base rotation about the camera (more like our rotate)
  const vec3f t = vec3f(delta.x, -delta.y, 0.f) * worldDiag;
  const vec3f worldt = 0.1f * xfmVector(cs.cameraToWorld, t);
  cs.centerTranslation  = AffineSpace3f::translate(worldt) * cs.centerTranslation;
  updateCamera();
}

vec3f ArcballCamera::eyePos() const
{
  return cs.cameraToWorld.p;
}

void ArcballCamera::setCenter(const vec3f &newCenter)
{
  cs.centerTranslation = AffineSpace3f::translate(-newCenter);
  updateCamera();
}

vec3f ArcballCamera::center() const
{
  return -cs.centerTranslation.p;
}

vec3f ArcballCamera::lookDir() const
{
  return xfmVector(cs.cameraToWorld, vec3f(0, 0, -1));
}

vec3f ArcballCamera::upDir() const
{
  return lockUpDir ? upVec : xfmVector(cs.cameraToWorld, vec3f(0, 1, 0));
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
  const AffineSpace3f rot           = LinearSpace3f(cs.rotation);
  const AffineSpace3f worldToCamera = cs.translation * rot * cs.centerTranslation;
  cs.cameraToWorld = rcp(worldToCamera);
}

void ArcballCamera::setRotation(quaternionf q)
{
  cs.rotation = q;
  updateCamera();
}

void ArcballCamera::setState(const CameraState &state)
{
  if (state.useCameraToWorld) {
    cs.cameraToWorld = state.cameraToWorld;
  } else {
    cs.centerTranslation = state.centerTranslation;
    cs.translation = state.translation;
    cs.rotation = state.rotation;
    updateCamera();
  }
}

float ArcballCamera::getZoomLevel()
{
  return cs.translation.p.z;
}

void ArcballCamera::setZoomLevel(float zoomLevel)
{
  cs.translation.p.z = zoomLevel;
  updateCamera();
}

CameraState ArcballCamera::getState() const
{
  return cs;
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