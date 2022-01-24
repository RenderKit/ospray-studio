// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "rkcommon/utility/StringManip.h"
#include "sg/Node.h"

using NodePtr = ospray::sg::NodePtr;
using WeakNodePtr = std::weak_ptr<ospray::sg::Node>;

using rkcommon::utility::beginsWith;

class AdvancedMaterialEditor
{
 public:
  AdvancedMaterialEditor() {}

  void buildUI(NodePtr materialRegistry, NodePtr &selectedMat);

 private:
  std::vector<std::string> currentMaterialTextureNames;

  inline NodePtr copyMaterial(NodePtr sourceMat,
      const std::string name = "",
      const std::string skipParam = "")
  {
    std::string newName = (name == "") ? sourceMat->name() : name;
    auto newMat = ospray::sg::createNode(newName, sourceMat->subType());

    for (auto &c : sourceMat->children()) {
      if (c.second->name() != "handles" && c.second->name() != skipParam) {
        // keep existing textures
        if (beginsWith(c.second->name(), std::string("map_")))
          newMat->add(c.second);
        else
          if (newMat->hasChild(c.second->name()))
            newMat->child(c.second->name()).setValue(c.second->value());
      }
    }

    return newMat;
   }

   inline bool clipboard()
   {
     // check if weak_ptr copiedMat is uninitialized
     // does not trigger if copiedMat is expired
     // https://stackoverflow.com/a/45507610/4765406
     return copiedMat.owner_before(WeakNodePtr{})
         || WeakNodePtr{}.owner_before(copiedMat);
   }
   WeakNodePtr copiedMat;

   inline void updateTextureNames(NodePtr currentMat)
   {
     currentMaterialTextureNames.clear();
     for (auto child : currentMat->children())
       if (beginsWith(child.first, std::string("map_")))
           currentMaterialTextureNames.push_back(child.first);
   }
};
