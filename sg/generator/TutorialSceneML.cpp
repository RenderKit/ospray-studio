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

  struct TutorialSceneML : public Generator
  {
    TutorialSceneML()           = default;
    ~TutorialSceneML() override = default;

    void generateData() override;
  };

  OSP_REGISTER_SG_NODE_NAME(TutorialSceneML, generator_multilevel_hierarchy);

  // TutorialSceneML definitions ////////////////////////////////////////////
  void makeScene(NodePtr nodeTop, NodePtr geometryX, bool useMultilevel)
  {
    NodePtr node = nodeTop;
    auto size = 1.f;
    auto scaleFactor = 0.8f;
    float absScale = scaleFactor;
    float x = 0.f;
    for (auto i = 0; i < 10; i++) {
      std::string nodeName  = "xfm_" + std::to_string(i);

      auto xfm = affine3f{one};
      if (useMultilevel) { // Relative to last xfm
        xfm = affine3f::scale(vec3f(scaleFactor)) * xfm;
        xfm = affine3f::translate(vec3f(x * size, 0, 0)) * xfm;
        x = (1.f + scaleFactor);
      } else { // Absolute
        xfm = affine3f::scale(vec3f(absScale)) * xfm;
        xfm = affine3f::translate(vec3f(x * size, 0, 0)) * xfm;
        x += (1.f + scaleFactor) * absScale;
        absScale *= scaleFactor;
      }

      auto newX = createNode(nodeName, "Transform", xfm);
      newX->add(geometryX);
      node->add(newX);
      if (useMultilevel)
        node = newX;
    }
  }

  void TutorialSceneML::generateData()
  {
    remove("singleLevel");
    remove("multiLevel");

    auto geomX = createNode("geomXfm", "Transform", affine3f{one});

    auto makeSpheres = true;
    if (makeSpheres) {
      // create a single sphere, then instance it in 2 ways.
      auto sphereG = createNode("spheres", "geometry_spheres");
      vec3f center{0.f};
      float radius = 1.f;
      sphereG->createChildData("sphere.position", center);
      sphereG->child("radius") = radius;
      geomX->add(sphereG);
    } else {
      // create boxes
      box3f boxes = {vec3f(-1.f,-1.f,-1.f),vec3f(1.f,1.f,1.f)};
      auto boxG = createNode("box", "geometry_boxes");
      boxG->createChildData("box", boxes);
      geomX->add(boxG);
    }

    auto singleLevel = createNode("single-level", "Transform", affine3f::translate(vec3f(0.f, 2.f, 0.f)));
    auto multiLevel = createNode("multi-level", "Transform", affine3f::translate(vec3f(0.f, -2.f, 0.f)));

    // Create 2 hierarchies, instancing the geometry multiple times in each
    // In singleLevel, all transforms hang off the passed-in node.
    // In multiLevel, each new transforms hangs off the previously created instance.
    makeScene(singleLevel, geomX, false);
    makeScene(multiLevel, geomX, true);

    // Add both methods to the scene
    add(singleLevel);
    add(multiLevel);
  }

  }  // namespace sg
} // namespace ospray
