// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "sg/Node.h"

namespace ospray {
  namespace sg {

  struct OSPSG_INTERFACE FrameBuffer
      : public OSPNode<cpp::FrameBuffer, NodeType::FRAME_BUFFER>
  {
    FrameBuffer();
    ~FrameBuffer() override = default;

    NodeType type() const override;

    const void *map(OSPFrameBufferChannel = OSP_FB_COLOR);
    void unmap(const void *mem);
    float variance();

    void resetAccumulation();
    void updateDenoiser(bool enabled);
    void saveFrame(std::string filename, int flags);

    inline bool hasFloatFormat()
    {
      return (child("colorFormat").valueAs<std::string>() == "float");
    }

    inline bool hasDepthChannel()
    {
      return (channels & OSP_FB_DEPTH);
    }

    inline bool hasAlbedoChannel()
    {
      return (channels & OSP_FB_ALBEDO);
    }

   private:
    void postCommit() override;

    void updateHandle();
    uint32_t channels{OSP_FB_COLOR};  // OSPFrameBufferChannel
    bool hasDenoiser{false};

    std::map<std::string, OSPFrameBufferFormat> colorFormats{
        {"sRGB", OSP_FB_SRGBA},
        {"RGBA8", OSP_FB_RGBA8},
        {"float", OSP_FB_RGBA32F},
        {"none", OSP_FB_NONE}};
  };

  }  // namespace sg
} // namespace ospray
