// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Generator.h"
// std
#include <random>

namespace ospray {
namespace sg {

struct TutorialSceneML : public Generator
{
  TutorialSceneML();
  ~TutorialSceneML() override = default;

  void generateData() override;
};

OSP_REGISTER_SG_NODE_NAME(TutorialSceneML, generator_multilevel_hierarchy);

// TutorialSceneML definitions ////////////////////////////////////////////
void makeScene(
    NodePtr nodeTop, NodePtr geometryX, float scaleFactor, bool useMultilevel)
{
  NodePtr node = nodeTop;
  float absScale = scaleFactor;
  float x = 0.f;
  for (auto i = 0; i < 10; i++) {
    std::string nodeName = "xfm_" + std::to_string(i);

    auto xfm = affine3f{one};
    if (useMultilevel) { // Relative to last xfm
      xfm = affine3f::scale(vec3f(scaleFactor)) * xfm;
      xfm = affine3f::translate(vec3f(x, 0, 0)) * xfm;
      x = (1.f + scaleFactor);
    } else { // Absolute
      xfm = affine3f::scale(vec3f(absScale)) * xfm;
      xfm = affine3f::translate(vec3f(x, 0, 0)) * xfm;
      x += (1.f + scaleFactor) * absScale;
      absScale *= scaleFactor;
    }

    auto newX = createNode(nodeName, "transform", xfm);
    newX->add(geometryX);
    node->add(newX);
    if (useMultilevel)
      node = newX;
  }
}

TutorialSceneML::TutorialSceneML()
{
  auto &parameters = child("parameters");
  parameters.createChild("SphereRadius", "float", 1.f);
  parameters.createChild("BoxSize", "float", 1.f);
  parameters.createChild("scaleFactor", "float", .8f);
  parameters["SphereRadius"].setMinMax(0.f, 3.f);
  parameters["BoxSize"].setMinMax(0.f, 3.f);
  parameters["scaleFactor"].setMinMax(0.f, 3.f);

  auto &xfm = createChild("xfm", "transform");
}

void TutorialSceneML::generateData()
{
  auto &parameters = child("parameters");
  auto radius = parameters["SphereRadius"].valueAs<float>();
  auto b = parameters["BoxSize"].valueAs<float>();
  auto scaleFactor = parameters["scaleFactor"].valueAs<float>();

  auto &xfm = child("xfm");

  auto sphereX = createNode("sphereXfm", "transform");
  auto boxX = createNode("boxXfm", "transform");

  // create a single sphere geometry, then instance it
  auto sphereG = createNode("spheres", "geometry_spheres");
  sphereG->createChildData("sphere.position", vec3f(0.f));
  sphereG->child("radius") = radius;
  // Assign the scenegraph default material
  sphereG->createChild("material", "uint32_t", (uint32_t)0);
  sphereG->child("material").setSGOnly();
  sphereX->add(sphereG);

  // create a single box geometry, then instance it
  box3f boxes = {box3f({vec3f(-b, -b, -b), vec3f(b, b, b)})};
  auto boxG = createNode("box", "geometry_boxes");
  boxG->createChildData("box", boxes);
  // Assign the scenegraph default material
  boxG->createChild("material", "uint32_t", (uint32_t)0);
  boxG->child("material").setSGOnly();
  boxX->add(boxG);

  auto singleLevel = createNode(
      "single-level", "transform", affine3f::translate(vec3f(0.f, 1.f, 0.f)));
  auto multiLevel = createNode(
      "multi-level", "transform", affine3f::translate(vec3f(0.f, -1.f, 0.f)));

  // Create 2 hierarchies, instancing the geometry multiple times in each
  // In singleLevel, all transforms hang off the passed-in node.
  // In multiLevel, new transforms hang off the previously created instance.
  makeScene(singleLevel, sphereX, scaleFactor, false);
  makeScene(multiLevel, boxX, scaleFactor, true);

  // Add both methods to the scene
  xfm.add(singleLevel);
  xfm.add(multiLevel);
}

} // namespace sg
} // namespace ospray
