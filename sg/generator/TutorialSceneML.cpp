// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Generator.h"
// std
#include <random>

namespace ospray {
namespace sg {

struct TutorialSceneML : public Generator
{
  TutorialSceneML() = default;
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
    std::string nodeName = "xfm_" + std::to_string(i);

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

  const std::vector<uint32_t> mID = {0};

  auto sphereX = createNode("sphereXfm", "Transform", affine3f{one});
  auto boxX = createNode("boxXfm", "Transform", affine3f{one});

  // create a single sphere geometry, then instance it
  auto sphereG = createNode("spheres", "geometry_spheres");
  vec3f center{0.f};
  float radius = 1.f;
  sphereG->createChildData("sphere.position", center);
  sphereG->child("radius") = radius;
  sphereG->createChildData("material", mID); // This is a scenegraph parameter
  sphereG->child("material").setSGOnly();
  sphereX->add(sphereG);

  // create a single box geometry, then instance it
  box3f boxes = {vec3f(1.f, 1.f, 1.f), vec3f(-1.f, -1.f, -1.f)};
  auto boxG = createNode("box", "geometry_boxes");
  boxG->createChildData("box", boxes);
  boxG->createChildData("material", mID); // This is a scenegraph parameter
  boxG->child("material").setSGOnly();
  boxX->add(boxG);

  auto singleLevel = createNode(
      "single-level", "Transform", affine3f::translate(vec3f(0.f, 1.f, 0.f)));
  auto multiLevel = createNode(
      "multi-level", "Transform", affine3f::translate(vec3f(0.f, -1.f, 0.f)));

  // Create 2 hierarchies, instancing the geometry multiple times in each
  // In singleLevel, all transforms hang off the passed-in node.
  // In multiLevel, new transforms hang off the previously created instance.
  makeScene(singleLevel, sphereX, false);
  makeScene(multiLevel, boxX, true);

  // Add both methods to the scene
  add(singleLevel);
  add(multiLevel);
}

} // namespace sg
} // namespace ospray
