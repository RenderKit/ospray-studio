// Copyright 2009-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "TransferFunction.h"

namespace ospray {
namespace sg {

struct OSPSG_INTERFACE Cloud : public TransferFunction
{
  Cloud();
  virtual ~Cloud() override = default;
};

OSP_REGISTER_SG_NODE_NAME(Cloud, transfer_function_cloud);

// Cloud definitions ////////////////////////////////////////////////////////

Cloud::Cloud() : TransferFunction("piecewiseLinear")
{
  colors.clear();
  colors = {vec3f(1.f)};
  createChildData("color", colors);
}

} // namespace sg
} // namespace ospray
