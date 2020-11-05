// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Generator.h"
// std
#include <random>

namespace ospray {
  namespace sg {

    struct TutorialScene : public Generator
    {
      TutorialScene()           = default;
      ~TutorialScene() override = default;

      void generateData() override;
    };

    OSP_REGISTER_SG_NODE_NAME(TutorialScene, generator_tutorial_scene);

    // TutorialScene definitions //////////////////////////////////////////////

    void TutorialScene::generateData()
    {
      remove("xfm");

      std::vector<vec3f> vertex = {vec3f(-1.0f, -1.0f, 3.0f),
                                   vec3f(-1.0f, 1.0f, 3.0f),
                                   vec3f(1.0f, -1.0f, 3.0f),
                                   vec3f(0.1f, 0.1f, 0.3f)};

      std::vector<vec4f> color = {vec4f(0.9f, 0.5f, 0.5f, 1.0f),
                                  vec4f(0.8f, 0.8f, 0.8f, 1.0f),
                                  vec4f(0.8f, 0.8f, 0.8f, 1.0f),
                                  vec4f(0.5f, 0.9f, 0.5f, 1.0f)};

      std::vector<vec3ui> index = {vec3ui(0, 1, 2), vec3ui(1, 2, 3)};

      auto &xfm = createChild("xfm", "transform");
      xfm["translation"] = vec3f(0.1f);

      // create and setup model and mesh
      auto &mesh = xfm.createChild("mesh", "geometry_triangles");

      mesh.createChildData("vertex.position", vertex);
      mesh.createChildData("vertex.color", color);
      mesh.createChildData("index", index);

      const std::vector<uint32_t> mID = {0};
      mesh.createChildData("material", mID); // This is a scenegraph parameter
      mesh.child("material").setSGOnly();
    }

  }  // namespace sg
}  // namespace ospray
