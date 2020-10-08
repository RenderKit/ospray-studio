// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "../Node.h"
#include "rkcommon/os/FileName.h"
#include "sg/renderer/MaterialRegistry.h"
#include "sg/texture/Texture2D.h"

#include "../../app/ospStudio.h"

namespace ospray {
namespace sg {

struct OSPSG_INTERFACE Importer : public Node
{
  Importer();
  virtual ~Importer() = default;

  NodeType type() const override;

  virtual void importScene();

  inline const rkcommon::FileName getFileName() const
  {
    return fileName;
  }

  inline void setFileName(rkcommon::FileName _fileName)
  {
    fileName = _fileName;
  }

  inline void setMaterialRegistry(
      std::shared_ptr<sg::MaterialRegistry> _registry)
  {
    materialRegistry = _registry;
  }

  inline void setTimesteps(std::vector<float> &_timesteps)
  {
    timesteps = &_timesteps;
    animate = true;
  }

  inline void setCameraList(std::vector<NodePtr> &_cameras)
  {
    cameras = &_cameras;
    importCameras = true;
  }

 protected:
  rkcommon::FileName fileName;
  std::shared_ptr<sg::MaterialRegistry> materialRegistry = nullptr;
  std::vector<float> *timesteps = nullptr;
  std::vector<NodePtr> *cameras = nullptr;
  bool importCameras{false};
  bool animate{false};
};

inline float dot(const quaternionf &q0, const quaternionf &q1)
{
  return q0.r * q1.r + q0.i * q1.i + q0.j * q1.j + q0.k * q1.k;
}

inline quaternionf slerp(const quaternionf &q0, const quaternionf &q1, float t)
{
  quaternionf qt0 = q0, qt1 = q1;
  float d = dot(qt0, qt1);
  if (d < 0.f) {
    // prevent "long way around"
    qt0 = -qt0;
    d = -d;
  } else if (d > 0.9995) {
    // angles too small
    quaternionf l = lerp(t, q0, q1);
    return normalize(l);
  }

  float theta = std::acos(d);
  float thetat = theta * t;

  float s0 = std::cos(thetat) - d * std::sin(thetat) / std::sin(theta);
  float s1 = std::sin(thetat) / std::sin(theta);

  return s0 * qt0 + s1 * qt1;
}

extern OSPSG_INTERFACE std::map<std::string, std::string> importerMap;

inline std::string getImporter(rkcommon::FileName fileName)
{
  auto fnd = importerMap.find(fileName.ext());
  if (fnd == importerMap.end()) {
    std::cout << "No importer for " << fileName << std::endl;
    return "";
  }

  std::string importer = fnd->second;
  std::string nodeName = "importer" + fileName.base();

  // auto &node = createNodeAs<sg::Importer>(nodeName, importer);
  // child("file") = fileName.base();
  return importer;
}

// for loading scene (.sg) files
OSPSG_INTERFACE void importScene(
    std::shared_ptr<StudioContext> context, rkcommon::FileName &fileName);

} // namespace sg
} // namespace ospray
