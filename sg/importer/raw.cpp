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

  // Keep this object alive for the duration of any lambdas
  auto self = shared_from_this();

  auto name = "load raw volume from "s + fileName.str();
  scheduler->background()->push(name, [&, self](SchedulerPtr scheduler) {
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
      for (auto &c : volumeParams->children()) {
        // Need to make a copy of the volume parameter here. If multiple threads
        // are using the same VolumeParams children objects, then because of the
        // book keeping involved with a node remembering its parents, multiple
        // threads could modify the parents vector within a Node object.
        //
        // Example: Threads "foo" and "bar" are running at the same time.
        // First, Foo adds a VolumeParams child to its own Volume object. Foo
        // recognizes that it will need to resize the Node::properties::parents
        // vector. Foo allocates a new parents vector. Next, Bar follows the
        // same process and allocates a new parents vector. Foo deallocates the
        // old pointer and so does Bar, leading to a double-free.
        //
        // The actual reason this happens is because although the
        // Importer::volumeParams object is newly created each time
        // Importer::getImporter() is called, the children of each of the
        // separate Importer::volumeParams objects are all references to the
        // exact same Node object.

        // Preferably this code would be something like:
        //   volume->add(createNodeLike(c.second))
        auto &p = c.second;
        volume->createChild(p->name(), p->subType(), p->description(), p->value());
      }

      auto structuredVolume = std::static_pointer_cast<StructuredVolume>(volume);
      structuredVolume->load(fileName);
      volumeImport = structuredVolume;
    }

    auto tf = createNode("transferFunction", "transfer_function_turbo");
    auto valueRange = volumeImport->child("valueRange").valueAs<range1f>();
    tf->child("valueRange") = valueRange.toVec2();
    volumeImport->add(tf);

    rootNode->add(volumeImport);

    auto name = "add raw volume from "s + fileName.str() + " to scene"s;
    scheduler->ospray()->push(name, [&, self, rootNode](SchedulerPtr scheduler) {
      // Finally, add node hierarchy to importer parent
      add(rootNode);
    });
  });
}

} // namespace sg
} // namespace ospray
