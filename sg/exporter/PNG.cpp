// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "ImageExporter.h"
// rkcommon
#include "rkcommon/os/FileName.h"
// stb
#include "stb_image_write.h"

namespace ospray {
  namespace sg {

  struct PNGExporter : public ImageExporter
  {
    PNGExporter()  = default;
    ~PNGExporter() = default;

    void doExport() override;
  };

  OSP_REGISTER_SG_NODE_NAME(PNGExporter, exporter_png);

  // PNGExporter definitions //////////////////////////////////////////////////

  void PNGExporter::doExport()
  {
    auto file    = FileName(child("file").valueAs<std::string>());

    if (child("data").valueAs<const void *>() == nullptr) {
      std::cerr << "Warning: image data null; not exporting" << std::endl;
      return;
    }

    std::string format = child("format").valueAs<std::string>();
    if (format == "float") {
      std::cerr << "Warning: saving a 32-bit float buffer as PNG; color space "
                   "will be limited."
                << std::endl;
      floatToChar();
    }

    vec2i size = child("size").valueAs<vec2i>();
    const void *fb = child("data").valueAs<const void *>();
    stbi_flip_vertically_on_write(1);
    int res = stbi_write_png(file.c_str(), size.x, size.y, 4, fb, 4 * size.x);

    if (res == 0)
      std::cerr << "STBI error; could not save image" << std::endl;
    else
      std::cout << "Saved to " << file << std::endl;
  }

  }  // namespace sg
} // namespace ospray
