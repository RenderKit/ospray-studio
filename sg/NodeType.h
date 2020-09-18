// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

namespace ospray {
  namespace sg {

  enum class NodeType
  {
    GENERIC,
    PARAMETER,
    FRAME,
    FRAME_BUFFER,
    RENDERER,
    CAMERA,
    WORLD,
    INSTANCE,
    ANIMATION,
    TRANSFORM,
    TRANSFER_FUNCTION,
    MATERIAL,
    MATERIAL_REFERENCE,
    TEXTURE,
    TEXTUREVOLUME,
    LIGHT,
    LIGHTS,
    GEOMETRY,
    VOLUME,
    GENERATOR,
    IMPORTER,
    EXPORTER,
    UNKNOWN = 9999
  };

  }  // namespace sg
} // namespace ospray
