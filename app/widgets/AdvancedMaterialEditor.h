// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "sg/Node.h"

using WeakNodePtr = std::weak_ptr<ospray::sg::Node>;

class AdvancedMaterialEditor
{
 public:
   AdvancedMaterialEditor() {}

   void buildUI(ospray::sg::NodePtr materialRegistry);
 private:
   inline bool clipboard()
   {
     // check if weak_ptr copiedMat is uninitialized
     // does not trigger if copiedMat is expired
     // https://stackoverflow.com/a/45507610/4765406
     return copiedMat.owner_before(WeakNodePtr{})
         || WeakNodePtr{}.owner_before(copiedMat);
   }
   WeakNodePtr copiedMat;
};
