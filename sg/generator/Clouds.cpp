// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Generator.h"
#include "sg/scene/volume/Vdb.h"
// std
#include <random>
// rkcommon
#include "rkcommon/tasking/parallel_for.h"

namespace ospray {
  namespace sg {

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
  }

  void Clouds::generateData()
  {
    remove("clouds");

    auto &xfm =
        createChild("clouds", "Transform", affine3f::translate(vec3f(0.0f)));

    auto &sky = xfm.createChild("sky", "sunsky");
    sky.createChild("intensity", "float", 1.f);
    sky.createChild("direction", "vec3f", vec3f(0.f, -0.75f, -1.f));
    sky.createChild("turbidity", "float", 2.f);

    auto &tf = xfm.createChild("transferFunction", "transfer_function_cloud");

    auto vol =
        std::static_pointer_cast<sg::VdbVolume>(sg::createNode("volume", "volume_vdb"));
    vol->load("/home/johannes/gfx/wdas_cloud/wdas_cloud.vdb");
    vol->createChild("densityScale", "float", 1.f);
    vol->createChild("anisotropy", "float", 0.6f);
    tf.add(vol);
  }

  }  // namespace sg
} // namespace ospray
