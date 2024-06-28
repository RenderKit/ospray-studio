// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Camera.h"

namespace ospray {
namespace sg {

struct Panoramic : public Camera
{
  Panoramic();
};

Panoramic::Panoramic() : Camera("panoramic")
{
  createChild("stereoMode",
      "OSPStereoMode",
      "0=none, 1=left, 2=right, 3=side-by-side, 4=top-bottom",
      OSP_STEREO_NONE);
  createChild("interpupillaryDistance",
      "float",
      "Distance between left and right eye for stereo mode",
      0.0635f);

  child("stereoMode").setMinMax(OSP_STEREO_NONE, OSP_STEREO_TOP_BOTTOM);
  child("interpupillaryDistance").setMinMax(0.f, 0.1f);
}

OSP_REGISTER_SG_NODE_NAME(Panoramic, camera_panoramic);

} // namespace sg
} // namespace ospray
