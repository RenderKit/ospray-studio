// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Importer.h"
// ospcommon
#include "rkcommon/os/FileName.h"
#include "../scene/volume/StructuredSpherical.h"

namespace ospray {
  namespace sg {

  struct RawImporter : public Importer
  {
    RawImporter()           = default;
    ~RawImporter() override = default;

    void importScene() override;
  };

  OSP_REGISTER_SG_NODE_NAME(RawImporter, importer_raw);
 
  // rawImporter definitions /////////////////////////////////////////////

  void RawImporter::importScene()
  {
    auto file    = FileName(child("file").valueAs<std::string>());

    std::string baseName = file.name() + '_';

    // Create a root Transform/Instance off the Importer, under which to build
    // the import hierarchy
    auto &tf = createChild(baseName + "root_node_tfn", "transfer_function_jet");
    
    std::shared_ptr<sg::StructuredSpherical> volumeImport =
        std::static_pointer_cast<sg::StructuredSpherical>(
            sg::createNode("volume", "structuredSpherical"));

    volumeImport->load(file);

    tf.add(volumeImport);

    std::cout << "...finished import!\n";
  }

  }  // namespace sg
} // namespace ospray
