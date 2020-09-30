// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Importer.h"

namespace ospray {
namespace sg {

OSPSG_INTERFACE std::map<std::string, std::string> importerMap = {
    {"obj", "importer_obj"},
    {"gltf", "importer_gltf"},
    {"glb", "importer_gltf"},
    {"raw", "importer_raw"},
    {"vdb", "importer_vdb"}};

Importer::Importer() {}

NodeType Importer::type() const
{
  return NodeType::IMPORTER;
}

void Importer::importScene() {}

} // namespace sg
} // namespace ospray
