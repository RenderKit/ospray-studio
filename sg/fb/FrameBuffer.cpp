// ======================================================================== //
// Copyright 2009-2019 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#include "FrameBuffer.h"
#include "../exporter/ImageExporter.h"

namespace ospray::sg {

  FrameBuffer::FrameBuffer()
  {
    createChild("size", "vec2i", vec2i(1024, 768));
    child("size").setReadOnly();
    createChild("colorFormat", "string", std::string("float"));

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

  void FrameBuffer::postCommit()
  {
    updateHandle();
  }

  void FrameBuffer::updateHandle()
  {
    auto size           = child("size").valueAs<vec2i>();
    auto colorFormatStr = child("colorFormat").valueAs<std::string>();

    auto fb = cpp::FrameBuffer(
        size.x, size.y,
        colorFormats[colorFormatStr],
        OSP_FB_COLOR | OSP_FB_ACCUM | OSP_FB_ALBEDO | OSP_FB_VARIANCE | OSP_FB_DEPTH);

    setHandle(fb);
  }

  void FrameBuffer::updateDenoiser(bool enabled)
  {
    // Denoiser requires float color buffer.
    auto colorFormatStr = child("colorFormat").valueAs<std::string>();
    if (enabled && colorFormatStr == "float") {
      cpp::ImageOperation d("denoiser");
      handle().setParam("imageOperation", cpp::CopiedData(d));
    } else {
      handle().removeParam("imageOperation");
    }
    handle().commit();
  }

  void FrameBuffer::saveFrame(std::string filename, bool depth)
  {
    auto exporter = getExporter(FileName(filename));
    if (exporter == "") {
      std::cout << "No exporter found for type " << FileName(filename).ext() << std::endl;
      return;
    }
    auto &exp = createChildAs<ImageExporter>("exporter", exporter);
    exp["file"] = filename;

    auto fb    = map(OSP_FB_COLOR);
    auto size  = child("size").valueAs<vec2i>();
    auto fmt   = child("colorFormat").valueAs<std::string>();
    void *zbuf = nullptr;

    exp.setImageData(fb, size, fmt);
    if (depth) {
      zbuf = (void *)map(OSP_FB_DEPTH);
      exp.setDepthData(zbuf);
    } else {
      exp.clearDepthData();
    }

    exp.doExport();

    unmap(fb);
    if (depth)
      unmap(zbuf);
  }

  OSP_REGISTER_SG_NODE_NAME(FrameBuffer, framebuffer);

}  // namespace ospray::sg
