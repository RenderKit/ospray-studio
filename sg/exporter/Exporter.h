// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../Node.h"
#include "rkcommon/os/FileName.h"

namespace ospray {
  namespace sg {

  struct OSPSG_INTERFACE Exporter : public Node
  {
    Exporter();
    virtual ~Exporter() = default;

    NodeType type() const override;

    virtual void doExport() {}

    uint32_t *_instData;
    uint16_t *_geomData;
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

  inline std::string getExporter(rkcommon::FileName fileName)
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

  }  // namespace sg
} // namespace ospray
