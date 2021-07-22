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
    {
      auto &vol = child("volume").valueAs<cpp::Volume>();
      auto &tf = child("transferFunction").valueAs<cpp::TransferFunction>();

      auto &me = valueAs<cpp::Texture>();
      me.setParam("volume", vol);
      me.setParam("transferFunction", tf);
      me.commit();
    }

    void TextureVolume::postCommit()
    {}

    OSP_REGISTER_SG_NODE_NAME(TextureVolume, texture_volume);

  } // namespace sg
} // namespace ospray
