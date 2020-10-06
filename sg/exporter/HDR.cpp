// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "ImageExporter.h"
// rkcommon
#include "rkcommon/os/FileName.h"
// stb
#include "stb_image_write.h"

namespace ospray {
  namespace sg {

  struct HDRExporter : public ImageExporter
  {
    HDRExporter()  = default;
    ~HDRExporter() = default;

    void doExport() override;
  };

  OSP_REGISTER_SG_NODE_NAME(HDRExporter, exporter_hdr);

  // HDRExporter definitions //////////////////////////////////////////////////

  void HDRExporter::doExport()
  {
    auto file    = FileName(child("file").valueAs<std::string>());

    if (child("data").valueAs<const void *>() == nullptr) {
      std::cerr << "Warning: image data null; not exporting" << std::endl;
      return;
    }

    std::string format = child("format").valueAs<std::string>();
    if (format != "float") {
      std::cerr << "Warning: saving a char buffer as HDR; image will not have "
                   "wide gamut."
                << std::endl;
      charToFloat();
    }

    vec2i size = child("size").valueAs<vec2i>();
    const void *fb = child("data").valueAs<const void *>();
    stbi_flip_vertically_on_write(1);
    int res =
        stbi_write_hdr(file.c_str(), size.x, size.y, 4, (const float *)fb);

    if (res == 0)
      std::cerr << "STBI error; could not save image" << std::endl;
    else
      std::cout << "Saved to " << file << std::endl;
  }

  }  // namespace sg
} // namespace ospray

