// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "MaterialReference.h"

namespace ospray {
  namespace sg {

  MaterialReference::MaterialReference()
  {
    setValue(0);
  }

  NodeType MaterialReference::type() const
  {
    return NodeType::MATERIAL_REFERENCE;
  }

  OSP_REGISTER_SG_NODE_NAME(MaterialReference, reference_to_material);

  }  // namespace sg
} // namespace ospray
