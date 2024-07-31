// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "sg/Node.h"

namespace ospray {
  namespace sg {

  struct OSPSG_INTERFACE FrameBuffer
      : public OSPNode<cpp::FrameBuffer, NodeType::FRAME_BUFFER>
  {
    FrameBuffer();

    NodeType type() const override;

    const void *map(OSPFrameBufferChannel = OSP_FB_COLOR);
    void unmap(const void *mem);
    float variance();

    void resetAccumulation();
    void updateDenoiser(bool enabled, bool finalFrame);
    void updateToneMapper(bool enabled);
    void updateImageOperations();
    void saveFrame(std::string filename, int flags);

    inline bool isFloatFormat()
    {
      return (child("colorFormat").valueAs<std::string>() == "float");
    }

    inline bool isSRGB()
    {
      return (hasChild("sRGB") && child("sRGB").valueAs<bool>());
    }

    inline bool hasDepthChannel()
    {
      return (channels & OSP_FB_DEPTH);
    }
    inline bool hasAccumChannel()
    {
      return (channels & OSP_FB_ACCUM);
    }
    inline bool hasVarianceChannel()
    {
      return (channels & OSP_FB_VARIANCE);
    }
    inline bool hasNormalChannel()
    {
      return (channels & OSP_FB_NORMAL);
    }
    inline bool hasAlbedoChannel()
    {
      return (channels & OSP_FB_ALBEDO);
    }
    inline bool hasPrimitiveIDChannel()
    {
      return (channels & OSP_FB_ID_PRIMITIVE);
    }
    inline bool hasObjectIDChannel()
    {
      return (channels & OSP_FB_ID_OBJECT);
    }
    inline bool hasInstanceIDChannel()
    {
      return (channels & OSP_FB_ID_INSTANCE);
    }

    inline NodePtr getToneMapper()
    {
      return toneMapper;
    }
    inline NodePtr getDenoiser()
    {
      return denoiser;
    }

   private:
    void postCommit() override;

    void updateHandle();
    uint32_t channels{OSP_FB_COLOR};  // OSPFrameBufferChannel

    bool hasDenoiser{false};
    bool hasToneMapper{false};
    bool updateImageOps{false};

    NodePtr toneMapper{nullptr};
    NodePtr denoiser{nullptr};
    OSPDenoiserQuality denoiserQuality{OSP_DENOISER_QUALITY_MEDIUM};

    std::map<std::string, OSPFrameBufferFormat> colorFormats{
        {"sRGB", OSP_FB_SRGBA},
        {"RGBA8", OSP_FB_RGBA8},
        {"float", OSP_FB_RGBA32F},
        {"none", OSP_FB_NONE}};
  };

  }  // namespace sg
} // namespace ospray
