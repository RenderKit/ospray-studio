// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Light.h"

namespace ospray {
  namespace sg {

  struct OSPSG_INTERFACE DirectionalLight : public Light
  {
    DirectionalLight();
    virtual ~DirectionalLight() override = default;
  };

  OSP_REGISTER_SG_NODE_NAME(DirectionalLight, distant);

  // DirectionalLight definitions /////////////////////////////////////////////

  DirectionalLight::DirectionalLight() : Light("distant")
  {
    createChild("direction", "vec3f", vec3f(0.f, 0.f, 1.f));
    child("direction").setMinMax(-1.f, 1.f); // per component min/max
    createChild("angularDiameter", "float", 0.f);
    child("direction").setMinMax(0.f, 10.f);
  }

  }  // namespace sg
} // namespace ospray
