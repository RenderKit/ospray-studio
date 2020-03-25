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

#pragma once

#include "../Node.h"
#include "ospcommon/os/FileName.h"
#include "sg/renderer/MaterialRegistry.h"
#include  "sg/texture/Texture2D.h"

namespace ospray::sg {

  struct OSPSG_INTERFACE Importer : public Node
  {
    Importer();
    virtual ~Importer() = default;

    NodeType type() const override;

    virtual void importScene(
        std::shared_ptr<sg::MaterialRegistry> materialRegistry) = 0;
  };

  static const std::map<std::string, std::string> importerMap = {
      {"obj", "importer_obj"},
      {"gltf", "importer_gltf"},
      {"glb", "importer_gltf"}};

  inline std::string getImporter(ospcommon::FileName fileName)
  {
    auto fnd = importerMap.find(fileName.ext());
    if (fnd == importerMap.end()) {
      std::cout << "No importer for selected file, nothing to import!\n";
      return "";
    }

    std::string importer = fnd->second;
    std::string nodeName = "importer" + fileName.base();

    // auto &node = createNodeAs<sg::Importer>(nodeName, importer);
    // child("file") = fileName.base();
    return importer;
  }

}  // namespace ospray::sg
