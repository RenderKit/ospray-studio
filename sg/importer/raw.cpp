// ======================================================================== //
// Copyright 2009-2019 Intel Corporation                                    //
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

#include "Importer.h"
// ospcommon
#include "rkcommon/os/FileName.h"
#include "../scene/volume/StructuredSpherical.h"

namespace ospray::sg {

  struct RawImporter : public Importer
  {
    RawImporter()           = default;
    ~RawImporter() override = default;

    void importScene();
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

}  // namespace ospray::sg
