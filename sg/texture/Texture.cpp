// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Texture.h"
#include "Texture2D.h"

namespace ospray {
  namespace sg {

  Texture::Texture(const std::string &type)
  {
    auto handle = ospNewTexture(type.c_str());
    setHandle(handle);
    createChild("filename", "string", "texture filename", std::string(""));
    child("filename").setSGOnly();
  }

  NodeType Texture::type() const
  {
    return NodeType::TEXTURE;
  }

  }  // namespace sg
} // namespace ospray
