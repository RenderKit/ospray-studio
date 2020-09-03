// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Light.h"

namespace ospray {
  namespace sg {

  struct OSPSG_INTERFACE SpotLight : public Light
  {
    SpotLight();
    virtual ~SpotLight() override = default;
  };

  OSP_REGISTER_SG_NODE_NAME(SpotLight, spot);

  // SpotLight definitions /////////////////////////////////////////////

  SpotLight::SpotLight() : Light("spot")
  {
    createChild("position", "vec3f", vec3f(0.f));   
    createChild("direction", "vec3f", vec3f(0.f, 0.f, 1.f));
    child("direction").setMinMax(-1.f, 1.f); // per component min/max
    createChild("openingAngle", "float", 180.f);
    child("openingAngle").setMinMax(0.f, 180.f);
    createChild("penumbraAngle", "float", 5.f);
    child("penumbraAngle").setMinMax(0.f, 180.f);
    createChild("radius", "float", 0.f);
  }

  }  // namespace sg
} // namespace ospray
