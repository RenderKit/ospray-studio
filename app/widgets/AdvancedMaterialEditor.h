// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "sg/Node.h"

class AdvancedMaterialEditor
{
 public:
   AdvancedMaterialEditor() {}

   void buildUI(ospray::sg::NodePtr materialRegistry);
 private:
   ospray::sg::NodePtr copiedMat;
};
