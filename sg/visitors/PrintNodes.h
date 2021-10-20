// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

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

  // uchar/uint8_t is only useful printed as an int
  static std::ostream &operator<<(std::ostream &cout, const uint8_t &uc)
  {
    return cout << (int)uc;
  }

  static std::ostream &operator<<(std::ostream &cout, const std::shared_ptr<Data> data)
  {
    return cout << "numItems=" << data->numItems
                << ", byteStride=" << data->byteStride
                << ", format=" << (int)data->format
                << ", isShared=" << (int)data->isShared;
  }

  inline bool PrintNodes::operator()(Node &node, TraversalContext &ctx)
  {
    std::cout << std::string(2*ctx.level, ' ') << node.name() << " : " << node.subType();

    // A couple of usings to make subType strings match types
    using string = std::string;
    using transform = affine3f;
    using uchar = uint8_t;
    PRINT_AS(node, string);      
    PRINT_AS(node, bool);      
    PRINT_AS(node, float);     
    PRINT_AS(node, vec2f);     
    PRINT_AS(node, vec3f);
    PRINT_AS(node, vec4f);
    PRINT_AS(node, quaternionf);
    PRINT_AS(node, uchar);       
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
    PRINT_AS(node, transform); 
    if (node.subType() == "Data")
      std::cout << " [" << node.nodeAs<Data>() << "]";
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
