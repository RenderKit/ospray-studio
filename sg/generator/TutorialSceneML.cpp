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

namespace ospray::sg {

  struct TutorialSceneML : public Generator
  {
    TutorialSceneML()           = default;
    ~TutorialSceneML() override = default;

    void generateData() override;
  };

  OSP_REGISTER_SG_NODE_NAME(TutorialSceneML, generator_multilevel_hierarchy);

  // TutorialSceneML definitions ////////////////////////////////////////////
  void makeScene(NodePtr nodeTop, NodePtr geometryNode, bool useMultilevel)
  {
    NodePtr node = nodeTop;
    auto scale = 1.f;
    auto x = 0.f;
    for (auto i = 0; i < 10; i++) {
      std::string nodeName  = "xfm_" + std::to_string(i);
      auto xfm = affine3f{one};
      xfm = affine3f::scale(vec3f(scale)) * xfm;
      xfm = affine3f::translate(vec3f(x, 0, 0)) * xfm;
      x += scale;
      scale *= 0.8;
      x += scale;
      auto newNode = createNode(nodeName, "Transform", xfm);
      newNode->add(geometryNode);
      node->add(newNode);
      if (useMultilevel)
        node = newNode;
    }
  }

  void TutorialSceneML::generateData()
  {
    remove("singleLevel");
    remove("multiLevel");

    // create a single sphere, then instance it in 2 ways.
    auto sphere = createNode("spheres", "geometry_spheres");
    vec3f center{0.f};
    float radius = 1.f;
    sphere->createChildData("sphere.position", center);
    sphere->child("radius") = radius;

    auto singleLevel = createNode("single-level", "Transform", affine3f::translate(vec3f(0.f, 2.f, 0.f)));
    auto multiLevel = createNode("multi-level", "Transform", affine3f::translate(vec3f(0.f, -2.f, 0)));

    // Create 2 hierarchies, instancing the sphere multiple times in each
    // In singleLevel, all transforms hang off the passed-in node.
    // In multiLevel, each new transforms hangs off the previously created instance.
    makeScene(singleLevel, sphere, false);
    makeScene(multiLevel, sphere, true);

    // Add both methods to the scene
    // Nears as I can tell, both hierarchies are created correctly, but only the
    // single level renders anything.  Not even the topmost sphere in multilevel
    // gets renderes.  (type 'p' to print the tree.)
    add(singleLevel);
    add(multiLevel);
  }
}  // namespace ospray::sg
