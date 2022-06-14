// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "Generator.h"
// std
#include <sg/scene/volume/Unstructured.h>
#include <random>

namespace ospray {
namespace sg {

struct UnstructuredVol : public Generator
{
  UnstructuredVol();
  ~UnstructuredVol() override = default;

  void generateData() override;
};

OSP_REGISTER_SG_NODE_NAME(UnstructuredVol, generator_unstructured_volume);

// UnstructuredVol definitions
// //////////////////////////////////////////////

UnstructuredVol::UnstructuredVol()
{
  auto &parameters = child("parameters");
  parameters.sgOnly();

  // define hexahedron parameters
  parameters.createChild("hSize", "float", .4f);
  parameters.createChild("hSpacing", "vec3f", vec3f(-.5f, -.5f, 0.f));
  // define wedge parameters
  parameters.createChild("wSize", "float", .4f);
  parameters.createChild("wSpacing", "vec3f", vec3f(.5f, -.5f, 0.f));
  // define tetrahedron parameters
  parameters.createChild("tSize", "float", .4f);
  parameters.createChild("tSpacing", "vec3f", vec3f(.5f, .5f, 0.f));
  // define pyramid parameters
  parameters.createChild("pSize", "float", .4f);
  parameters.createChild("pSpacing", "vec3f", vec3f(-.5f, .5f, 0.f));

  auto &xfm = createChild("xfm", "transform");
}

void UnstructuredVol::generateData()
{
  auto &xfm = child("xfm");
  auto &tf = xfm.createChild("transferFunction", "transfer_function_turbo");

  auto &parameters = child("parameters");

  auto hSize = parameters["hSize"].valueAs<float>();
  auto h = parameters["hSpacing"].valueAs<vec3f>();

  auto wSize = parameters["wSize"].valueAs<float>();
  auto w = parameters["wSpacing"].valueAs<vec3f>();

  auto tSize = parameters["tSize"].valueAs<float>();
  auto t = parameters["tSpacing"].valueAs<vec3f>();

  auto pSize = parameters["pSize"].valueAs<float>();
  auto p = parameters["pSpacing"].valueAs<vec3f>();

  // define vertex positions
  std::vector<vec3f> vertices = {// hexahedron
      {-hSize + h.x, -hSize + h.y, hSize + h.z}, // bottom quad
      {hSize + h.x, -hSize + h.y, hSize + h.z},
      {hSize + h.x, -hSize + h.y, -hSize + h.z},
      {-hSize + h.x, -hSize + h.y, -hSize + h.z},
      {-hSize + h.x, hSize + h.y, hSize + h.z}, // top quad
      {hSize + h.x, hSize + h.y, hSize + h.z},
      {hSize + h.x, hSize + h.y, -hSize + h.z},
      {-hSize + h.x, hSize + h.y, -hSize + h.z},

      // wedge
      {-wSize + w.x, -wSize + w.y, wSize + w.z}, // botom triangle
      {wSize + w.x, -wSize + w.y, 0.f + w.z},
      {-wSize + w.x, -wSize + w.y, -wSize + w.z},
      {-wSize + w.x, wSize + w.y, wSize + w.z}, // top triangle
      {wSize + w.x, wSize + w.y, 0.f + w.z},
      {-wSize + w.x, wSize + w.y, -wSize + w.z},

      // tetrahedron
      {-tSize + t.x, -tSize + t.y, tSize + t.z},
      {tSize + t.x, -tSize + t.y, 0.f + t.z},
      {-tSize + t.x, -tSize + t.y, -tSize + t.z},
      {-tSize + t.x, tSize + t.y, 0.f + t.z},

      // pyramid
      {-pSize + p.x, -pSize + p.y, pSize + p.z},
      {pSize + p.x, -pSize + p.y, pSize + p.z},
      {pSize + p.x, -pSize + p.y, -pSize + p.z},
      {-pSize + p.x, -pSize + p.y, -pSize + p.z},
      {pSize + p.x, pSize + p.y, 0.f + p.z}};

  // define per-vertex values
  std::vector<float> vertexValues = {
      // hexahedron
      0.f,
      0.f,
      0.f,
      0.f,
      0.f,
      1.f,
      1.f,
      0.f,

      // wedge
      0.f,
      0.f,
      0.f,
      1.f,
      0.f,
      1.f,

      // tetrahedron
      1.f,
      0.f,
      1.f,
      0.f,

      // pyramid
      0.f,
      1.f,
      1.f,
      0.f,
      0.f};

  // define vertex indices for both shared and separate case
  std::vector<uint32_t> indicesSharedVert = {
      // hexahedron
      0,
      1,
      2,
      3,
      4,
      5,
      6,
      7,

      // wedge
      1,
      9,
      2,
      5,
      12,
      6,

      // tetrahedron
      5,
      12,
      6,
      17,

      // pyramid
      4,
      5,
      6,
      7,
      17};

      std::vector<uint32_t> &indices = indicesSharedVert;

  // define cell offsets in indices array
  std::vector<uint32_t> cells = {0, 8, 14, 18};

  // define cell types
  std::vector<uint8_t> cellTypes = {
      OSP_HEXAHEDRON, OSP_WEDGE, OSP_TETRAHEDRON, OSP_PYRAMID};

  auto &volume = tf.createChild("unstructured_volume", "volume_unstructured");

  const auto minmax =
      std::minmax_element(begin(vertexValues), end(vertexValues));
  auto valueRange = range1f(*std::get<0>(minmax), *std::get<1>(minmax));
  volume["valueRange"] = valueRange;
  tf["valueRange"] = valueRange.toVec2();

  // set data objects for volume object
  volume["vertex.position"] = (cpp::CopiedData)vertices;
  volume["index"] = (cpp::CopiedData)indices;
  volume["cell.index"] = (cpp::CopiedData)cells;
  volume["vertex.data"] = (cpp::CopiedData)vertexValues;
  volume["cell.type"] = (cpp::CopiedData)cellTypes;
}
} // namespace sg
} // namespace ospray
