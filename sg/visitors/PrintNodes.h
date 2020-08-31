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
    std::cout << std::string(2*ctx.level, ' ') << node.name() << " : " << node.subType();

    // A couple of usings to make subType strings match types
    using string = std::string;
    using Transform = affine3f;
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
    PRINT_AS(node, Transform); 
    std::cout << std::endl;

    // XXX Debug only, probably need something better here.
    // Don't litter the PrintNodes with copies of instances, only fully traverse
    // the main one hanging off world.
    if (ctx.level > 2 && node.subType().find("importer_") != std::string::npos)
        return false;

    return true;
  }

  }  // namespace sg
} // namespace ospray
