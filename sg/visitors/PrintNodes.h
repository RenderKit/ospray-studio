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

namespace ospray::sg {

  struct PrintNodes : public Visitor
  {
    PrintNodes() = default;

    bool operator()(Node &node, TraversalContext &ctx) override;
  };

  // Inlined definitions //////////////////////////////////////////////////////

#define PRINT_AS(node, type) \
  if (node.subType() == #type) \
  std::cout << " [" << node.valueAs<type>() << "]";

  inline bool PrintNodes::operator()(Node &node, TraversalContext &ctx)
  {
    for (int i = 0; i < ctx.level; i++)
      std::cout << "  ";
    std::cout << node.name() << " : " << node.subType();

    using string = std::string;
    PRINT_AS(node, string);      
    PRINT_AS(node, bool);      
    PRINT_AS(node, float);     
    PRINT_AS(node, vec2f);     
    PRINT_AS(node, vec3f);     
    PRINT_AS(node, vec4f);     
    PRINT_AS(node, int);       
    PRINT_AS(node, vec2i);     
    PRINT_AS(node, vec3i);     
    PRINT_AS(node, vec4i);     
    PRINT_AS(node, void *);  
    PRINT_AS(node, box3f);     
    PRINT_AS(node, box3i);     
    PRINT_AS(node, range1f);   
    PRINT_AS(node, rgb);       
    PRINT_AS(node, rgba);      
    PRINT_AS(node, affine3f); 
    std::cout << std::endl;

    return true;
  }

}  // namespace ospray::sg
