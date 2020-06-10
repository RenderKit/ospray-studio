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
#include "ospcommon/utility/SaveImage.h"

namespace ospray::sg {

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

      ospcommon::utility::writePFM(
          file.c_str(), size.x, size.y, (const vec4f *)fb);
    } else {
      ospcommon::utility::writePPM(
          file.c_str(), size.x, size.y, (const uint32_t *)fb);
    }

    std::cout << "Saved to " << file << std::endl;
  }

}  // namespace ospray::sg

