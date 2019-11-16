// ======================================================================== //
// Copyright 2009-2019 Intel Corporation                                    //
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

namespace ospray {
  namespace sg {

    struct CommitVisitor : public Visitor
    {
      CommitVisitor() = default;
      ~CommitVisitor() override = default;

      bool operator()(Node &node, TraversalContext &) override;
      void postChildren(Node &node, TraversalContext &) override;
    };

    // Inlined definitions ////////////////////////////////////////////////////

    inline bool CommitVisitor::operator()(Node &node, TraversalContext &)
    {
      if (node.subtreeModifiedButNotCommitted()) {
        node.preCommit();
        return true;
      } else {
        return false;
      }
    }

    void CommitVisitor::postChildren(Node &node, TraversalContext &)
    {
      if (node.subtreeModifiedButNotCommitted()) {
        node.postCommit();
        node.properties.lastCommitted.renew();
      }
    }

  }  // namespace sg
}  // namespace ospray
