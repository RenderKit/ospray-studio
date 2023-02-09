// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Camera.h"

namespace ospray {
namespace sg {

struct Perspective : public Camera
{
  Perspective();
};

Perspective::Perspective() : Camera("perspective")
{
  createChild("fovy", "float", "Field-of-view in degrees", 60.f);
  createChild("aspect", "float", 1.f);
  createChild("apertureRadius",
      "float",
      "size of the aperture, controls the depth of field",
      0.f);
  createChild("focusDistance",
      "float",
      "distance at where the image is sharpest when depth of field is enabled",
      1.f);
  createChild("adjustAperture",
      "bool",
      "Automatically adjust aperture to maintain relative DoF when varying focalDistance",
      false);
  child("adjustAperture").setSGOnly();
  createChild("architectural",
      "bool",
      "vertical edges are projected to be parallel",
      false);
  createChild("stereoMode",
      "int",
      "[0=none, 1=left, 2=right, 3=side-by-side, 4=top-bottom]",
      0);
  createChild("interpupillaryDistance",
      "float",
      "Distance between left and right eye for stereo mode [default 0.0635]",
      0.0635f);

  child("fovy").setMinMax(0.f, 180.f);
  child("apertureRadius").setMinMax(0.f, 5.f);  // XXX set these based on
  child("focusDistance").setMinMax(1.f, 1e6f);  // world size
  child("stereoMode")
      .setMinMax((int)OSP_STEREO_NONE, (int)OSP_STEREO_TOP_BOTTOM);
  child("interpupillaryDistance").setMinMax(0.f, 0.1f);

  child("aspect").setReadOnly();
  
  createChild("offAxisMode",
      "bool",
      "toggle off-axis projection mode",
      false);
  createChild("transform", "affine3f", affine3f(one));
  createChild("topLeft", "vec3f", vec3f{-10.0f, 10.0f, -20.0f});
  createChild("botLeft", "vec3f", vec3f{-10.0f, -10.0f, -20.0f});
  createChild("botRight", "vec3f", vec3f{10.0f, -10.0f, -20.0f});
}

OSP_REGISTER_SG_NODE_NAME(Perspective, camera_perspective);

} // namespace sg
} // namespace ospray
