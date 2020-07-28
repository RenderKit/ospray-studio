// Copyright 2018-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Camera.h"

namespace ospray {
namespace sg {

  struct Orthographic : public Camera
  {
    Orthographic();
  };

  Orthographic::Orthographic() : Camera("orthographic")
  {
    createChild("height", "float", "size of camera's image plane in y", 60.f);
    child("height").setMinMax(0.f, 180.f);
    createChild("aspect", "float", 1.f);
    child("aspect").setReadOnly();
  }

  OSP_REGISTER_SG_NODE_NAME(Orthographic, camera_orthographic);

} // namespace sg
} // namespace ospray
