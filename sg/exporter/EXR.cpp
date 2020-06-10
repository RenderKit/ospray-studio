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
#include "ImfChannelList.h"
#include "ImfOutputFile.h"

namespace ospray::sg {

  struct EXRExporter : public ImageExporter
  {
    EXRExporter()  = default;
    ~EXRExporter() = default;

    void doExport() override;
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
    namespace IMF = OPENEXR_IMF_NAMESPACE;
    namespace IMATH = IMATH_NAMESPACE;

    Imf::Header exrHeader(size.x, size.y);
    exrHeader.channels().insert("R", Imf::Channel(IMF::FLOAT));
    exrHeader.channels().insert("G", Imf::Channel(IMF::FLOAT));
    exrHeader.channels().insert("B", Imf::Channel(IMF::FLOAT));
    exrHeader.channels().insert("A", Imf::Channel(IMF::FLOAT));

    Imf::FrameBuffer exrFb;
    // generic API requires channels individually
    // in particular, we need to use the Slice::Make function because we have a
    // const pointer, and the Slice constructor only takes non-const >:|
    exrFb.insert("R",
                 Imf::Slice::Make(IMF::FLOAT,
                                  (const void *)((const float *)fb + 0),
                                  IMATH::V2i(0),
                                  size.x,
                                  size.y,
                                  sizeof(float) * 4,
                                  size.x * sizeof(float) * 4));
    exrFb.insert("G",
                 Imf::Slice::Make(IMF::FLOAT,
                                  (const void *)((const float *)fb + 1),
                                  IMATH::V2i(0),
                                  size.x,
                                  size.y,
                                  sizeof(float) * 4,
                                  size.x * sizeof(float) * 4));
    exrFb.insert("B",
                 Imf::Slice::Make(IMF::FLOAT,
                                  (const void *)((const float *)fb + 2),
                                  IMATH::V2i(0),
                                  size.x,
                                  size.y,
                                  sizeof(float) * 4,
                                  size.x * sizeof(float) * 4));
    exrFb.insert("A",
                 Imf::Slice::Make(IMF::FLOAT,
                                  (const void *)((const float *)fb + 3),
                                  IMATH::V2i(0),
                                  size.x,
                                  size.y,
                                  sizeof(float) * 4,
                                  size.x * sizeof(float) * 4));

    Imf::OutputFile exrFile(file.c_str(), exrHeader);
    exrFile.setFrameBuffer(exrFb);
    exrFile.writePixels(size.y);

    std::cout << "Saved to " << file << std::endl;
  }

}  // namespace ospray::sg

