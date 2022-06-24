// Copyright 2018 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "imGuIZMOquat.h"

#include "Guizmo.h"

namespace ospray_studio {

// XXX Example of a manipulator gizmo

void guizmo()
{
  // For imGuIZMO, declare static or global variable or member class quaternion
  static quat qRot = quat(1.f, 0.f, 0.f, 0.f);
  ImGui::gizmo3D("##gizmo1", qRot /*, size,  mode */);
}

} // namespace ospray_studio
