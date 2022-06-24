// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "sg/Node.h"

namespace ospray {
namespace sg {

struct OSPSG_INTERFACE Light : public OSPNode<cpp::Light, NodeType::LIGHT>
{
  Light(std::string type);
  ~Light() override = default;
  NodeType type() const override;

  // Lights are either in the World lights list or in a group list.
  bool inGroup{false};

 protected:
  void preCommit() override;
  void addMeasuredSource(std::string fileName = "");
};

} // namespace sg
} // namespace ospray
