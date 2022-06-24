// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "FrameBuffer.h"
#include "../exporter/ImageExporter.h"

#include "sg/camera/Camera.h"
#include "sg/renderer/Renderer.h"
#include "sg/scene/World.h"

#include "sg/Mpi.h"

namespace ospray {
namespace sg {

FrameBuffer::FrameBuffer()
{
  createChild("floatFormat",
      "bool",
      "framebuffer needs float format and channels compatible with denoising",
      false);
  createChild("size", "vec2i", vec2i(1024, 768));
  child("size").setReadOnly();
  createChild("colorFormat",
      "string",
      "framebuffer format: RBGA8, sRGBA, or float",
      std::string("RGBA8"));

  createChild("exposure", "float", "amount of light per unit area", 1.0f);
  createChild("contrast",
      "float",
      "contrast (toe of the curve); typically is in [1-2]",
      1.1759f);
  createChild("shoulder",
      "float",
      "highlight compression (shoulder of the curve); typically is in [0.9-1]",
      0.9746f);
  createChild(
      "midIn", "float", "mid-level anchor input; default is 18%% gray", 0.18f);
  createChild("midOut",
      "float",
      "mid-level anchor output; default is 18%% gray",
      0.18f);
  createChild(
      "hdrMax", "float", "maximum HDR input that is not clipped", 6.3704f);
  createChild("acesColor", "bool", "apply the ACES color transforms", true);

  child("exposure").setMinMax(0.f, 5.f);
  child("contrast").setMinMax(0.f, 3.f);
  child("shoulder").setMinMax(0.f, 2.f);
  child("midIn").setMinMax(0.f, 1.f);
  child("midOut").setMinMax(0.f, 1.f);
  child("hdrMax").setMinMax(0.f, 100.f);

  updateHandle();
}

NodeType FrameBuffer::type() const
{
  return NodeType::FRAME_BUFFER;
}

const void *FrameBuffer::map(OSPFrameBufferChannel channel)
{
  return handle().map(channel);
}

void FrameBuffer::unmap(const void *mem)
{
  handle().unmap(const_cast<void *>(mem));
}

void FrameBuffer::resetAccumulation()
{
  handle().clear();
}

float FrameBuffer::variance()
{
  return handle().variance();
}

void FrameBuffer::postCommit()
{
  updateHandle();
}

void FrameBuffer::updateHandle()
{
  // Default minimal format
  channels = OSP_FB_COLOR | OSP_FB_ACCUM | OSP_FB_VARIANCE;

  // Denoising requires float format and additional channels
  auto floatFormat = child("floatFormat").valueAs<bool>();
  if (floatFormat) {
    child("colorFormat") = std::string("float");
    remove("sRGB");
    channels |= OSP_FB_ALBEDO | OSP_FB_DEPTH | OSP_FB_NORMAL;
  } else {
    if (!hasChild("sRGB"))
      createChild("sRGB", "bool", "sRGB encoded framebuffer", true);

    auto sRGB = child("sRGB").valueAs<bool>();
    child("colorFormat") = std::string(sRGB ? "sRGB" : "RGBA8");
  }

  auto size = child("size").valueAs<vec2i>();
  // Assure that neither dimension is 0.
  size = vec2i(std::max(size.x, 1), std::max(size.y, 1));
  auto colorFormatStr = child("colorFormat").valueAs<std::string>();

  auto fb =
      cpp::FrameBuffer(size.x, size.y, colorFormats[colorFormatStr], channels);

  setHandle(fb);

  // Recreating the framebuffer will change the imageOps.  Refresh them.
  if (hasDenoiser || hasToneMapper) {
    updateImageOps = true;
    updateImageOperations();
  }
}

void FrameBuffer::updateDenoiser(bool enabled)
{
  // Denoiser requires float color buffer.
  if (!isFloatFormat() || (enabled == hasDenoiser))
    return;

  hasDenoiser = enabled;
  updateImageOps = true;
}

void FrameBuffer::updateToneMapper(bool enabled)
{
  // ToneMapper requires float color buffer.
  if (!isFloatFormat() || (enabled == hasToneMapper))
    return;

  hasToneMapper = enabled;
  updateImageOps = true;
}

void FrameBuffer::updateImageOperations()
{
  if (sgUsingMpi() && sgMpiRank() != 0)
      return;

  // Only update imageOperation if necessary
  if (!updateImageOps)
    return;

  updateImageOps = false;

  std::vector<cpp::ImageOperation> ops = {};
  if (hasToneMapper) {
    auto iop = cpp::ImageOperation("tonemapper");
    float exposure = child("exposure").valueAs<float>();
    iop.setParam("exposure", OSP_FLOAT, &exposure);
    float contrast = child("contrast").valueAs<float>();
    iop.setParam("contrast", OSP_FLOAT, &contrast);
    float shoulder = child("shoulder").valueAs<float>();
    iop.setParam("shoulder", OSP_FLOAT, &shoulder);
    float midIn = child("midIn").valueAs<float>();
    iop.setParam("midIn", OSP_FLOAT, &midIn);
    float midOut = child("midOut").valueAs<float>();
    iop.setParam("midOut", OSP_FLOAT, &midOut);
    float hdrMax = child("hdrMax").valueAs<float>();
    iop.setParam("hdrMax", OSP_FLOAT, &hdrMax);
    bool acesColor = child("acesColor").valueAs<bool>();
    iop.setParam("acesColor", OSP_BOOL, &acesColor);
    iop.commit();
    ops.push_back(iop);
  }
  if (hasDenoiser)
    ops.push_back(cpp::ImageOperation("denoiser"));

  if (isFloatFormat() && (hasDenoiser || hasToneMapper))
    handle().setParam("imageOperation", cpp::CopiedData(ops));
  else
    handle().removeParam("imageOperation");
  handle().commit();
}

void FrameBuffer::saveFrame(std::string filename, int flags)
{
  if (sgUsingMpi() && sgMpiRank() != 0)
      return;

  std::vector<std::string> filenames;
  auto file = FileName(filename);
  
  if (flags && file.ext() != "exr") {
    auto exrFile = file.dropExt().str() + ".exr";
    filenames.push_back(exrFile);
  }
  filenames.push_back(filename);

  for (auto &f : filenames) {
    auto exporter = getExporter(FileName(f));

    if (exporter == "") {
      std::cout << "No exporter found for type " << FileName(filename).ext()
                << std::endl;
      return;
    }
    auto exp = createNodeAs<ImageExporter>("exporter", exporter);
    exp->child("file") = f;

    auto fb = map(OSP_FB_COLOR);
    auto size = child("size").valueAs<vec2i>();
    auto fmt = child("colorFormat").valueAs<std::string>();
    void *abuf = nullptr;
    void *zbuf = nullptr;
    void *nbuf = nullptr;

    exp->setImageData(fb, size, fmt);

    bool albedo = flags & 0b1;
    bool depth = flags & 0b10;
    bool normal = flags & 0b100;
    bool layersAsSeparateFiles = flags & 0b1000;

    if (albedo) {
      abuf = (void *)map(OSP_FB_ALBEDO);
      exp->setAdditionalLayer("albedo", abuf);
    } else {
      exp->clearLayer("albedo");
    }

    if (depth) {
      zbuf = (void *)map(OSP_FB_DEPTH);
      exp->setAdditionalLayer("Z", zbuf);
    } else {
      exp->clearLayer("Z");
    }

    if (normal) {
      nbuf = (void *)map(OSP_FB_NORMAL);
      exp->setAdditionalLayer("normal", nbuf);
    } else {
      exp->clearLayer("normal");
    }

    exp->createChild("layersAsSeparateFiles", "bool", layersAsSeparateFiles);
    exp->createChild("saveColor", "bool", file.ext() == "exr");

    exp->doExport();

    unmap(fb);
    if (abuf)
      unmap(abuf);
    if (zbuf)
      unmap(zbuf);
    if (nbuf)
      unmap(nbuf);
  }
}

OSP_REGISTER_SG_NODE_NAME(FrameBuffer, framebuffer);

} // namespace sg
} // namespace ospray
