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

#pragma once

#include "../Node.h"
#include "ospcommon/os/FileName.h"

namespace ospray::sg {

  struct OSPSG_INTERFACE Exporter : public Node
  {
    Exporter();
    virtual ~Exporter() = default;

    NodeType type() const override;

    virtual void doExport() {}
  };

  static const std::map<std::string, std::string> exporterMap = {
      {"png", "exporter_png"},
      {"jpg", "exporter_jpg"},
      {"ppm", "exporter_ppm"},
      {"pfm", "exporter_ppm"},
#ifdef STUDIO_OPENEXR
      {"exr", "exporter_exr"},
#endif
      {"hdr", "exporter_hdr"},
  };

  inline std::string getExporter(ospcommon::FileName fileName)
  {
    auto fnd = exporterMap.find(fileName.ext());
    if (fnd == exporterMap.end())
      return "";
    else
      return fnd->second;
  }

  // currently used to get screenshot filetypes
  // assumes that exporterMap only contains image types
  inline std::vector<std::string> getExporterTypes()
  {
    std::vector<std::string> expTypes;
    for (const auto &e : exporterMap)
      expTypes.push_back(e.first);
    std::sort(expTypes.begin(), expTypes.end());
    return expTypes;
  }

}  // namespace ospray::sg
