// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

namespace ospray {
namespace sg {

// Wrap NodeList in preprocessor foo so that we can create to/fromString maps
#define NODELIST                                                               \
  X(GENERIC)                                                                   \
  X(PARAMETER)                                                                 \
  X(FRAME)                                                                     \
  X(FRAME_BUFFER)                                                              \
  X(RENDERER)                                                                  \
  X(CAMERA)                                                                    \
  X(WORLD)                                                                     \
  X(TRANSFORM)                                                                 \
  X(TRANSFER_FUNCTION)                                                         \
  X(MATERIAL)                                                                  \
  X(MATERIAL_REFERENCE)                                                        \
  X(TEXTURE)                                                                   \
  X(TEXTUREVOLUME)                                                             \
  X(LIGHT)                                                                     \
  X(LIGHTS)                                                                    \
  X(GEOMETRY)                                                                  \
  X(VOLUME)                                                                    \
  X(GENERATOR)                                                                 \
  X(IMPORTER)                                                                  \
  X(EXPORTER)                                                                  \
  X(UNKNOWN)

enum class NodeType
{
#define X(node) node,
  NODELIST
#undef X
};

static std::map<NodeType, std::string> NodeTypeToString = {
#define X(node) {ospray::sg::NodeType::node, #node},
    NODELIST
#undef X
};

static std::map<std::string, NodeType> NodeTypeFromString = {
#define X(node) {#node, ospray::sg::NodeType::node},
    NODELIST
#undef X
};

} // namespace sg
} // namespace ospray
