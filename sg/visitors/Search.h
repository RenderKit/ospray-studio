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

namespace ospray {
  namespace sg {

    struct Search : public Visitor
    {
      Search(const std::string &s, std::vector<NodePtr> &v);

      bool operator()(Node &node, TraversalContext &ctx) override;

     private:
      std::string term;
      std::vector<NodePtr> &results;
    };

    // Inlined definitions ////////////////////////////////////////////////////

    Search::Search(const std::string &s, std::vector<NodePtr> &v)
        : term(s), results(v)
    {
    }

    inline bool Search::operator()(Node &node, TraversalContext &ctx)
    {
      if (node.name().find(term) != std::string::npos) {
        results.push_back(NodePtr(&node));
      }
      return true;
    }
  }  // namespace sg
}  // namespace ospray
