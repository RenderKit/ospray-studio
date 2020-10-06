// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Instance.h"

namespace ospray {
  namespace sg {

  Instance::Instance()
  {
    cpp::Instance inst(group);
    setHandle(inst);
    xfms.emplace(math::one);
  }

  NodeType Instance::type() const
  {
    return NodeType::INSTANCE;
  }

  void Instance::preCommit()
  {
    auto matId = child("materialRef").valueAs<int>();
    materialIDs.push(matId);
  }

  void Instance::postCommit()
  {
    auto &geom = child("generator").child("geometry").valueAs<cpp::Geometry>();
    cpp::GeometricModel geomModel(geom);
    geomModel.setParam("material", materialIDs.top());
    geomModel.commit();
    group.setParam("geometry", (cpp::Data)geomModel);
    group.commit();

    auto &inst = handle();
    inst.setParam("xfm", xfms.top());
    inst.commit();
  }

  OSP_REGISTER_SG_NODE_NAME(Instance, instance);

  }  // namespace sg
} // namespace ospray
