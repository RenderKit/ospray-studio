// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../Node.h"

namespace ospray {
  namespace sg {

    struct OSPSG_INTERFACE Material : public Node
    {
      Material(std::string type);
      virtual ~Material() override = default;

      NodeType type() const override;

      std::string osprayMaterialType() const;

    private:
      void preCommit() override;
      void postCommit() override;

      std::string matType;
    };

  }  // namespace sg
}  // namespace ospray
