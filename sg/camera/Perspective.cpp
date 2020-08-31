// Copyright 2009-2020 Intel Corporation
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
    child("fovy").setMinMax(0.f, 180.f);
    createChild("aspect", "float", 1.f);
    child("aspect").setReadOnly();
    createChild("interpupillaryDistance",
                "float",
                "Distance between left and right eye for stereo mode",
                0.0635f);
    child("interpupillaryDistance").setMinMax(0.f, 0.1f);
    createChild("stereoMode",
                "int",
                "0=none, 1=left, 2=right, 3=side-by-side, 4=top-bottom",
                0);
  }

  OSP_REGISTER_SG_NODE_NAME(Perspective, camera_perspective);

  }  // namespace sg
} // namespace ospray
