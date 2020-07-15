// ======================================================================== //
// Copyright 2020 Intel Corporation                                         //
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

#include "Generator.h"
#include "sg/scene/volume/Vdb.h"
// std
#include <random>
// rkcommon
#include "rkcommon/tasking/parallel_for.h"

namespace ospray::sg {

  struct Clouds : public Generator
  {
    Clouds();
    ~Clouds() override = default;

    void generateData() override;
  };

  OSP_REGISTER_SG_NODE_NAME(Clouds, generator_clouds);

  // Clouds definitions ////////////////////////////////////////////////////////

  Clouds::Clouds()
  {
    auto &parameters = child("parameters");
  }

  void Clouds::generateData()
  {
    remove("clouds");

    auto &xfm =
        createChild("clouds", "Transform", affine3f::translate(vec3f(0.0f)));

    auto &sky = xfm.createChild("sky", "sunsky");
    sky.createChild("intensity", "float", 1.f);
    sky.createChild("direction", "vec3f", vec3f(0.f, -0.25f, -1.f));

    auto &tf = xfm.createChild("transferFunction", "transfer_function_cloud");

    auto vol =
        std::static_pointer_cast<sg::VdbVolume>(sg::createNode("volume", "volume_vdb"));
    vol->load("/home/johannes/gfx/wdas_cloud/wdas_cloud.vdb");
    vol->createChild("densityScale", "float", 1.f);
    vol->createChild("anisotropy", "float", 0.6f);
    tf.add(vol);
  }

}  // namespace ospray::sg
