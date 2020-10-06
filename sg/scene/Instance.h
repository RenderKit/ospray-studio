// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../Node.h"
#include <stack>

namespace ospray {
  namespace sg {

    struct OSPSG_INTERFACE Instance : public OSPNode<cpp::Instance, NodeType::INSTANCE>
    {
      Instance();
      virtual ~Instance() override = default;

      NodeType type() const override;

      void preCommit() override;
      void postCommit() override;

      std::stack<affine3f> xfms;
      cpp::Group group;
      std::stack<uint32_t> materialIDs;
    };

  }  // namespace sg
}  // namespace ospray
