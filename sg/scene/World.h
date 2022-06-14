// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "sg/Node.h"

namespace ospray {
namespace sg {

struct OSPSG_INTERFACE World : public OSPNode<cpp::World, NodeType::WORLD>
{
  World();
  ~World() override = default;

  virtual void preCommit() override;
  virtual void postCommit() override;

  std::shared_ptr<OSPInstanceSGIdMap> instSGIdMap;
  std::shared_ptr<OSPGeomModelSGIdMap> geomSGIdMap;
};

} // namespace sg
} // namespace ospray
