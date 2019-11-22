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

    OSP_REGISTER_SG_NODE_NAME(TutorialScene, Generator_tutorialScene);
    OSP_REGISTER_SG_NODE_NAME(TutorialScene, Generator_TutorialScene);

    // TutorialScene definitions //////////////////////////////////////////////

    void TutorialScene::generateData()
    {
      removeAllChildren();

      std::vector<vec3f> vertex = {vec3f(-1.0f, -1.0f, 3.0f),
                                   vec3f(-1.0f, 1.0f, 3.0f),
                                   vec3f(1.0f, -1.0f, 3.0f),
                                   vec3f(0.1f, 0.1f, 0.3f)};

      std::vector<vec4f> color = {vec4f(0.9f, 0.5f, 0.5f, 1.0f),
                                  vec4f(0.8f, 0.8f, 0.8f, 1.0f),
                                  vec4f(0.8f, 0.8f, 0.8f, 1.0f),
                                  vec4f(0.5f, 0.9f, 0.5f, 1.0f)};

      std::vector<vec3ui> index = {vec3ui(0, 1, 2), vec3ui(1, 2, 3)};

      auto xfm = sg::createNode("xfm",
                                "Transform",
                                "affine transformation",
                                affine3f::translate(vec3f(0.1f)));

      // create and setup model and mesh
      auto mesh = sg::createNode("mesh", "Geometry_triangles", "triangle mesh");

      mesh->createChildData("vertex.position", vertex);
      mesh->createChildData("vertex.color", color);
      mesh->createChildData("index", index);

      xfm->add(mesh);

      this->add(xfm);
    }

  }  // namespace sg
}  // namespace ospray
