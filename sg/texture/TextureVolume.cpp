// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "TextureVolume.h"

namespace ospray {
  namespace sg {

    // TextureVolume definitions //////////////////////////////////////////////////
    TextureVolume::TextureVolume() : Texture("volume") {}

    NodeType TextureVolume::type() const
    {
      return NodeType::TEXTUREVOLUME;
    }

    void TextureVolume::preCommit()
    {}

    void TextureVolume::postCommit()
    {}

    OSP_REGISTER_SG_NODE_NAME(TextureVolume, textureVolume);

  } // namespace sg
} // namespace ospray
