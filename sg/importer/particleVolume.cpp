// Copyright 2009-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Importer.h"
// ospcommon
#include "../scene/volume/ParticleVolume.h"
#include "rkcommon/os/FileName.h"

namespace ospray {
namespace sg {

struct ParticleVolumeImporter : public Importer
{
  ParticleVolumeImporter() = default;
  ~ParticleVolumeImporter() override = default;

  void importScene() override;
};

OSP_REGISTER_SG_NODE_NAME(ParticleVolumeImporter, importer_pvol);

// ParticleImporter definitions /////////////////////////////////////////////

void ParticleVolumeImporter::importScene()
{
  std::cout << "ParticleVolumeImporter::importScene()" << std::endl;

  // Create a root Transform/Instance off the Importer, then place the volume
  // under this.
  auto rootName = fileName.name() + "_rootXfm";
  auto nodeName = fileName.name() + "_volume";

  auto last = fileName.base().find_last_of(".");
  auto volumeTypeExt = fileName.base().substr(last, fileName.base().length());

  auto rootNode = createNode(rootName, "transform");
  NodePtr volumeImport;

  auto volume = createNode(nodeName, "volume_particle");
  for (auto &c : volumeParams->children())
    volume->add(c.second);

  auto particleVolume = std::static_pointer_cast<ParticleVolume>(volume);
  particleVolume->load(fileName);
  volumeImport = particleVolume;

  auto tf = createNode("transferFunction", "transfer_function_turbo");
  auto valueRange = volumeImport->child("valueRange").valueAs<range1f>();
  tf->child("valueRange") = valueRange.toVec2();
  volumeImport->add(tf);

  rootNode->add(volumeImport);

  // Finally, add node hierarchy to importer parent
  add(rootNode);

  std::cout << "...finished import!\n";
}

} // namespace sg
} // namespace ospray
