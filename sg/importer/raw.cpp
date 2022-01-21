// Copyright 2009-2021 Intel Corporation
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
  using namespace std::string_literals;

  auto self = shared_from_this();

  std::vector<Node *> parents = this->parents();
  this->removeAllParents();

  scheduler->push(background, "RawImporter "s + fileName.name(), [&, self, this, parents](SchedulerPtr scheduler) {
    // Create a root Transform/Instance off the Importer, then place the volume
    // under this.
    auto rootName = fileName.name() + "_rootXfm";
    auto nodeName = fileName.name() + "_volume";

    auto last = fileName.base().find_last_of(".");
    auto volumeTypeExt = fileName.base().substr(last, fileName.base().length());

    auto rootNode = createNode(rootName, "transform");
    NodePtr volumeImport;

    if (volumeTypeExt == ".spherical") {
      auto volume = createNode(nodeName, "structuredSpherical");
      for (auto &c : volumeParams->children())
        volume->add(c.second);

      auto sphericalVolume =
          std::static_pointer_cast<StructuredSpherical>(volume);
      sphericalVolume->load(fileName);
      volumeImport = sphericalVolume;
    } else {
      auto volume = createNode(nodeName, "structuredRegular");
      for (auto &c : volumeParams->children())
        volume->add(c.second);

      auto structuredVolume = std::static_pointer_cast<StructuredVolume>(volume);
      structuredVolume->load(fileName);
      volumeImport = structuredVolume;
    }

    auto tf = createNode("transferFunction", "transfer_function_turbo");
    auto valueRange = volumeImport->child("valueRange").valueAs<range1f>();
    tf->child("valueRange") = valueRange.toVec2();
    volumeImport->add(tf);

    rootNode->add(volumeImport);

    // Finally, add node hierarchy to importer parent
    add(rootNode);

    scheduler->push(foreground, "RawImporter "s + fileName.name(), [&, self, this, parents](SchedulerPtr scheduler) {
      for (auto &parent : parents) {
        parent->add(*this);
      }
      std::cout << "...finished import!\n";
    });
  });
}

} // namespace sg
} // namespace ospray
