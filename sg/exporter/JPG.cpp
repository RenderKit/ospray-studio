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
// stb
#include "stb_image_write.h"

namespace ospray::sg {

  struct JPGExporter : public ImageExporter
  {
    JPGExporter()  = default;
    ~JPGExporter() = default;

    void doExport() override;
  };

  OSP_REGISTER_SG_NODE_NAME(JPGExporter, exporter_jpg);

  // JPGExporter definitions //////////////////////////////////////////////////

  void JPGExporter::doExport()
  {
    auto file    = FileName(child("file").valueAs<std::string>());

    if (child("data").valueAs<const void *>() == nullptr) {
      std::cerr << "Warning: image data null; not exporting" << std::endl;
      return;
    }

    std::string format = child("format").valueAs<std::string>();
    if (format == "float") {
      std::cerr << "Warning: saving a 32-bit float buffer as JPG; color space "
                   "will be limited."
                << std::endl;
      floatToChar();
    }

    vec2i size = child("size").valueAs<vec2i>();
    const void *fb = child("data").valueAs<const void *>();
    int res = stbi_write_jpg(file.c_str(), size.x, size.y, 4, fb, 90);

    if (res == 0)
      std::cerr << "STBI error; could not save image" << std::endl;
    else
      std::cout << "Saved to " << file << std::endl;
  }

}  // namespace ospray::sg

