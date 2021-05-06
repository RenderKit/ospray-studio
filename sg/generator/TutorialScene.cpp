// Copyright 2009-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Generator.h"
// std
#include <random>

namespace ospray {
namespace sg {

struct TutorialScene : public Generator
{
  TutorialScene();
  ~TutorialScene() override = default;

  void generateData() override;
};

OSP_REGISTER_SG_NODE_NAME(TutorialScene, generator_tutorial_scene);

// TutorialScene definitions //////////////////////////////////////////////

TutorialScene::TutorialScene()
{
  auto &parameters = child("parameters");
  parameters.createChild("V0", "vec3f", vec3f(-1.0f, -1.0f, 3.0f));
  parameters.createChild("V1", "vec3f", vec3f(-1.0f, 1.0f, 3.0f));
  parameters.createChild("V2", "vec3f", vec3f(1.0f, -1.0f, 3.0f));
  parameters.createChild("V3", "vec3f", vec3f(0.1f, 0.1f, 0.3f));
  parameters.child("V0").setMinMax(-3.f, 3.f);
  parameters.child("V1").setMinMax(-3.f, 3.f);
  parameters.child("V2").setMinMax(-3.f, 3.f);
  parameters.child("V3").setMinMax(-3.f, 3.f);

  parameters.createChild("C0", "rgba", vec4f(0.9f, 0.5f, 0.5f, 1.0f));
  parameters.createChild("C1", "rgba", vec4f(0.8f, 0.8f, 0.8f, 1.0f));
  parameters.createChild("C2", "rgba", vec4f(0.8f, 0.8f, 0.8f, 1.0f));
  parameters.createChild("C3", "rgba", vec4f(0.5f, 0.9f, 0.5f, 1.0f));

  auto &xfm = createChild("xfm", "transform");
}

void TutorialScene::generateData()
{
  auto &parameters = child("parameters");
  auto V0 = parameters["V0"].valueAs<vec3f>();
  auto V1 = parameters["V1"].valueAs<vec3f>();
  auto V2 = parameters["V2"].valueAs<vec3f>();
  auto V3 = parameters["V3"].valueAs<vec3f>();

  auto C0 = parameters["C0"].valueAs<rgba>();
  auto C1 = parameters["C1"].valueAs<rgba>();
  auto C2 = parameters["C2"].valueAs<rgba>();
  auto C3 = parameters["C3"].valueAs<rgba>();

  auto &xfm = child("xfm");
  xfm["translation"] = vec3f(0.1f);
  auto &mesh = xfm.createChild("mesh", "geometry_triangles");

  std::vector<vec3f> vertex = {V0, V1, V2, V3};
  std::vector<vec4f> color = {C0, C1, C2, C3};
  std::vector<vec3ui> index = {vec3ui(0, 1, 2), vec3ui(1, 2, 3)};

  mesh.createChildData("vertex.position", vertex);
  mesh.createChildData("vertex.color", color);
  mesh.createChildData("index", index);

  // Assign the scenegraph default material
  mesh.createChild("material", "uint32_t", (uint32_t)0);
  mesh.child("material").setSGOnly();
}

} // namespace sg
} // namespace ospray
