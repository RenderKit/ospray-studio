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

#include "Volume.h"

namespace ospray::sg {

  Volume::Volume(const std::string &osp_type)
  {
    setValue(cpp::Volume(osp_type));
  }

  NodeType Volume::type() const
  {
    return NodeType::VOLUME;
  }

  void Volume::preCommit()
  {
    auto &vol     = valueAs<cpp::Volume>();
    const auto &c = children();
    if (c.empty())
      return;
    for (auto &child : c) {
      if (child.second->type() == NodeType::PARAMETER) {
        child.second->setOSPRayParam(child.first, vol.handle());
      }
    }
  }

  void Volume::postCommit()
  {
    auto &vol = valueAs<cpp::Volume>();
    vol.commit();
  }


}  // namespace ospray::sg