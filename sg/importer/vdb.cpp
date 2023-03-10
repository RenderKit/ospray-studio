// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Importer.h"
// ospcommon
#include "rkcommon/os/FileName.h"
#include "sg/scene/volume/Vdb.h"

namespace ospray {
namespace sg {

struct VDBImporter : public Importer
{
  VDBImporter() = default;
  ~VDBImporter() override = default;

  void importScene() override;
};

OSP_REGISTER_SG_NODE_NAME(VDBImporter, importer_vdb);

// vdbImporter definitions /////////////////////////////////////////////

void VDBImporter::importScene()
{
#if USE_OPENVDB
  // Create a root Transform/Instance off the Importer, under which to build
  // the import hierarchy
  auto rootName = fileName.name() + "_rootXfm";
  auto nodeName = fileName.name() + "_volume";

  auto rootNode = createNode(rootName, "transform");
  auto volumeImport = createNodeAs<VdbVolume>(nodeName, "volume_vdb");

  volumeImport->load(fileName);

  auto tf = createNode("transferFunction", "transfer_function_turbo");
  auto valueRange = volumeImport->child("value").valueAs<range1f>();
  tf->child("value") = valueRange;
  volumeImport->add(tf);

  rootNode->add(volumeImport);

  // Finally, add node hierarchy to importer parent
  add(rootNode);

  std::cout << "...finished import!\n";
#else
  std::cout << "OpenVDB not enabled in build.  Rebuild Studio, selecting "
               "ENABLE_OPENVDB in cmake."
            << std::endl;
#endif
}

} // namespace sg
} // namespace ospray
