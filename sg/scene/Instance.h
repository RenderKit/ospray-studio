// ======================================================================== //
// Copyright 2020 Intel Corporation                                    //
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
