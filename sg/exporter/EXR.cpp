// ======================================================================== //
// Copyright 2020 Intel Corporation                                         //
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

#include "ImageExporter.h"
// ospcommon
#include "ospcommon/os/FileName.h"
// openexr
#include "OpenEXR/ImfChannelList.h"
#include "OpenEXR/ImfOutputFile.h"

namespace ospray::sg {

  struct EXRExporter : public ImageExporter
  {
    EXRExporter()  = default;
    ~EXRExporter() = default;

    void doExport() override;
    float *flipBuffer(const void *buf, int ncomp=4);
  };

  OSP_REGISTER_SG_NODE_NAME(EXRExporter, exporter_exr);

  // EXRExporter definitions //////////////////////////////////////////////////

  void EXRExporter::doExport()
  {
    auto file = FileName(child("file").valueAs<std::string>());

    if (child("data").valueAs<const void *>() == nullptr) {
      std::cerr << "Warning: image data null; not exporting" << std::endl;
      return;
    }

    std::string format = child("format").valueAs<std::string>();
    if (format != "float") {
      std::cerr << "Warning: saving a char buffer as EXR; image will not have "
                   "wide gamut."
                << std::endl;
      charToFloat();
    }

    vec2i size     = child("size").valueAs<vec2i>();
    const void *fb = child("data").valueAs<const void *>();

    // use general EXR file API for 32-bit float support
    namespace IMF   = OPENEXR_IMF_NAMESPACE;
    namespace IMATH = IMATH_NAMESPACE;

    Imf::Header exrHeader(size.x, size.y);
    exrHeader.channels().insert("R", Imf::Channel(IMF::FLOAT));
    exrHeader.channels().insert("G", Imf::Channel(IMF::FLOAT));
    exrHeader.channels().insert("B", Imf::Channel(IMF::FLOAT));
    exrHeader.channels().insert("A", Imf::Channel(IMF::FLOAT));

    auto makeSlice = [&](const void *fb, int offset, int ncomp = 4) {
      // flip the data
      return Imf::Slice(IMF::FLOAT,
                        (char *)((float *)fb + offset),
                        sizeof(float) * ncomp,
                        size.x * sizeof(float) * ncomp);
    };

    // although openexr provides a LineOrder parameter, it doesn't seem to
    // actually flip the image...
    float *flippedfb = flipBuffer(fb);
    float *flippedz  = nullptr;

    Imf::FrameBuffer exrFb;
    // generic API requires channels individually
    exrFb.insert("R", makeSlice(flippedfb, 0));
    exrFb.insert("G", makeSlice(flippedfb, 1));
    exrFb.insert("B", makeSlice(flippedfb, 2));
    exrFb.insert("A", makeSlice(flippedfb, 3));

    if (hasChild("depth")) {
      exrHeader.channels().insert("Z", Imf::Channel(IMF::FLOAT));
      const void *depth = child("depth").valueAs<const void *>();
      flippedz          = flipBuffer(depth, 1);
      exrFb.insert("Z", makeSlice(flippedz, 0, 1));
    }

    Imf::OutputFile exrFile(file.c_str(), exrHeader);
    exrFile.setFrameBuffer(exrFb);
    exrFile.writePixels(size.y);

    std::cout << "Saved to " << file << std::endl;

    free(flippedfb);
    if (flippedz != nullptr)
      free(flippedz);
  }

  float *EXRExporter::flipBuffer(const void *buf, int ncomp)
  {
    vec2i size = child("size").valueAs<vec2i>();
    float *flipped =
        (float *)std::malloc(size.x * size.y * ncomp * sizeof(float));

    for (int y = size.y - 1; y >= 0; y--) {
      size_t irow = y * size.x * ncomp;
      size_t orow = (size.y - 1 - y) * size.x * ncomp;
      std::memcpy(flipped + orow,
                  ((float *)buf) + irow,
                  size.x * ncomp * sizeof(float));
    }

    return flipped;
  }

}  // namespace ospray::sg

