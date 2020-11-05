// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Importer.h"
// ospcommon
#include "../scene/volume/Structured.h"
#include "../scene/volume/StructuredSpherical.h"
#include "rkcommon/os/FileName.h"

namespace ospray {
namespace sg {

struct RawImporter : public Importer
{
  RawImporter() = default;
  ~RawImporter() override = default;

  void importScene() override;
};

OSP_REGISTER_SG_NODE_NAME(RawImporter, importer_raw);

// rawImporter definitions /////////////////////////////////////////////

void RawImporter::importScene()
{
  // Create a root Transform/Instance off the Importer, then place the volume
  // under this.
  auto rootName = fileName.name() + "_rootXfm";
  auto nodeName = fileName.name() + "_volume";

  auto last = fileName.base().find_last_of(".");
  auto volumeTypeExt = fileName.base().substr(last, fileName.base().length());

  auto rootNode = createNode(rootName, "transform");
  NodePtr volumeImport;

  if (volumeTypeExt == ".spherical") {
    auto volume =
        createNodeAs<StructuredSpherical>(nodeName, "structuredSpherical");
    volume->load(fileName);
    volumeImport = volume;
  } else {
    auto volume = createNodeAs<StructuredVolume>(nodeName, "structuredRegular");
    volume->load(fileName);
    volumeImport = volume;
  }

  auto tf = createNode("transferFunction", "transfer_function_jet");
  volumeImport->add(tf);

  rootNode->add(volumeImport);

  // Finally, add node hierarchy to importer parent
  add(rootNode);

  std::cout << "...finished import!\n";
}

} // namespace sg
} // namespace ospray
