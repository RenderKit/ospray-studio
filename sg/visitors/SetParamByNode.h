// ======================================================================== //
// Copyright 2020 Intel Corporation                                         //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

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
                                           TraversalContext &ctx)
    {
      if (node.type() == type) {
        node.child(param).setValue(value);
        return false;
      }
      
      return true;
    }
  }  // namespace sg
}  // namespace ospray
