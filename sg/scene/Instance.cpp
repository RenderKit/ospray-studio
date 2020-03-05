// ======================================================================== //
// Copyright 2020 Intel Corporation                                    //
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

#include "Instance.h"

namespace ospray::sg {

  Instance::Instance()
  {
    cpp::Instance inst(group);
    setHandle(inst);
    xfms.emplace(math::one);
  }

  NodeType Instance::type() const
  {
    return NodeType::INSTANCE;
  }

  void Instance::preCommit()
  {
    auto matId = child("materialRef").valueAs<int>();
    materialIDs.push(matId);
  }

  void Instance::postCommit()
  {
    auto &geom = child("generator").child("geometry").valueAs<cpp::Geometry>();
    cpp::GeometricModel geomModel(geom);
    geomModel.setParam("material", materialIDs.top());
    geomModel.commit();
    group.setParam("geometry", (cpp::Data)geomModel);
    group.commit();

    auto &inst = handle();
    inst.setParam("xfm", xfms.top());
    inst.commit();
  }

  OSP_REGISTER_SG_NODE_NAME(Instance, instance);

}  // namespace ospray::sg
