// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../Node.h"
#include "rkcommon/utility/Any.h"

#include <string>

namespace ospray {
  namespace sg {

    struct SetParamByNode : public Visitor
    {
      SetParamByNode(NodeType targetType,
                     std::string targetParam,
                     rkcommon::utility::Any targetValue)
          : type(targetType), param(targetParam), value(targetValue)
      {
      }

      bool operator()(Node &node, TraversalContext &ctx) override;

     private:
      NodeType type;
      std::string param;
      rkcommon::utility::Any value;
    };

    // Inlined definitions ////////////////////////////////////////////////////

    inline bool SetParamByNode::operator()(Node &node,
                                           TraversalContext &)
    {
      if (node.type() == type) {
        node.child(param).setValue(value);
        return false;
      }
      
      return true;
    }
  }  // namespace sg
}  // namespace ospray
