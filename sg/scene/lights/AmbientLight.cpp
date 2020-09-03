// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Light.h"

namespace ospray {
  namespace sg {

  struct OSPSG_INTERFACE AmbientLight : public Light
  {
    AmbientLight();
    virtual ~AmbientLight() override = default;
  };

  OSP_REGISTER_SG_NODE_NAME(AmbientLight, ambient);

  // AmbientLight definitions /////////////////////////////////////////////

  AmbientLight::AmbientLight() : Light("ambient")
  {
    child("intensity").setMinMax(0.f, 1.f);

    // Ambient should be invisible by default, not directly viewable
    child("visible").setValue(false);
  }

  }  // namespace sg
} // namespace ospray
