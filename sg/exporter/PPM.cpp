// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "ImageExporter.h"
// rkcommon
#include "rkcommon/os/FileName.h"
#include "rkcommon/utility/SaveImage.h"

namespace ospray {
  namespace sg {

  struct PPMExporter : public ImageExporter
  {
    PPMExporter()  = default;
    ~PPMExporter() = default;

    void doExport() override;
  };

  OSP_REGISTER_SG_NODE_NAME(PPMExporter, exporter_ppm);

  // PPMExporter definitions //////////////////////////////////////////////////

  void PPMExporter::doExport()
  {
    auto file    = FileName(child("file").valueAs<std::string>());

    if (child("data").valueAs<const void *>() == nullptr) {
      std::cerr << "Warning: image data null; not exporting" << std::endl;
      return;
    }

    std::string format = child("format").valueAs<std::string>();
    vec2i size = child("size").valueAs<vec2i>();
    const void *fb = child("data").valueAs<const void *>();

    if (format == "float") {
      // TODO: there is a bug in FileName::setExt that deletes the dot when
      // replacing the extension. Fix and use that in rkcommon when we make the
      // switch
      // file = file.setExt("pfm");
      auto fn = child("file").valueAs<std::string>();
      auto dot = fn.find_last_of('.');
      if (dot == std::string::npos)
        file = FileName(fn + ".pfm");
      else
        file = FileName(fn.substr(0, dot+1) + "pfm");

      rkcommon::utility::writePFM(
          file.c_str(), size.x, size.y, (const vec4f *)fb);
    } else {
      rkcommon::utility::writePPM(
          file.c_str(), size.x, size.y, (const uint32_t *)fb);
    }

    std::cout << "Saved to " << file << std::endl;
  }

  }  // namespace sg
} // namespace ospray

