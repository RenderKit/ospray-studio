// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

// sg
#include "Texture.h"

namespace ospray {
  namespace sg{

    /*! \brief wrapper for a Texture Volume */
    struct OSPSG_INTERFACE TextureVolume : public Texture
    {
      /*! constructor */
      TextureVolume();
      virtual ~TextureVolume() override = default;

      NodeType type() const override;

      void preCommit() override;
      void postCommit() override;
    };

  } // namespace sg
} // namespace ospray
