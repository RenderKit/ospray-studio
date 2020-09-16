// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Importer.h"

namespace ospray {
namespace sg {

Importer::Importer() {}

NodeType Importer::type() const
{
  return NodeType::IMPORTER;
}

void Importer::importScene() {}

} // namespace sg
} // namespace ospray
