// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Camera.h"

namespace ospray::sg {

  struct Panoramic : public Camera
  {
    Panoramic();
  };

  Panoramic::Panoramic() : Camera("panoramic")
  {
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

  OSP_REGISTER_SG_NODE_NAME(Panoramic, camera_panoramic);

}  // namespace ospray::sg
