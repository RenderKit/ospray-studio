// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <json.hpp>

#include "Node.h"
#include "importer/Importer.h"

#include "../app/ArcballCamera.h"

// This file contains definitions of `to_json` and `from_json` for custom types
// used within Studio. These methods allow us to easily serialize and
// deserialize SG nodes to JSON.

// rkcommon type declarations /////////////////////////////////////////

namespace rkcommon {
namespace containers {
void to_json(
    nlohmann::json &j, const FlatMap<std::string, ospray::sg::NodePtr> &fm);
void from_json(
    const nlohmann::json &j, FlatMap<std::string, ospray::sg::NodePtr> &fm);
} // namespace containers
namespace utility {
void to_json(nlohmann::json &j, const Any &a);
void from_json(const nlohmann::json &j, Any &a);
} // namespace utility
} // namespace rkcommon

///////////////////////////////////////////////////////////////////////
// SG types ///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

namespace ospray {
namespace sg {

inline void to_json(nlohmann::json &j, const Node &n)
{
  j = nlohmann::json{{"name", n.name()},
      {"type", n.type()},
      {"subType", n.subType()},
      {"description", n.description()}};

  // we only want the importer and its root transform, not the hierarchy of
  // geometry under it
  if (n.type() == NodeType::IMPORTER) {
    j["filename"] = n.nodeAs<const Importer>()->getFileName().str();
    for (auto &child : n.children()) {
      if (child.second->type() == NodeType::TRANSFORM) {
        j["children"] = {*child.second};
        break;
      }
    }
    return;
  }

  if (n.value().valid() && (n.type() == NodeType::PARAMETER
      || n.type() == NodeType::TRANSFORM))
    j["value"] = n.value();

  if (n.hasChildren() && n.type() != NodeType::TRANSFORM)
    j["children"] = n.children();
}

inline void from_json(const nlohmann::json &j, Node &n) {}

} // namespace sg
} // namespace ospray

///////////////////////////////////////////////////////////////////////
// rkcommon type definitions //////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

namespace rkcommon {
namespace containers {

inline void to_json(
    nlohmann::json &j, const FlatMap<std::string, ospray::sg::NodePtr> &fm)
{
  for (const auto e : fm)
    j.push_back(*(e.second));
}

inline void from_json(
    const nlohmann::json &j, FlatMap<std::string, ospray::sg::NodePtr> &fm)
{}

} // namespace containers

namespace math {

inline void to_json(nlohmann::json &j, const vec2f &v)
{
  j = {v.x, v.y};
}

inline void from_json(const nlohmann::json &j, vec2f &v)
{
  j.at(0).get_to(v.x);
  j.at(1).get_to(v.y);
}

inline void to_json(nlohmann::json &j, const vec3f &v)
{
  j = {v.x, v.y, v.z};
}

inline void from_json(const nlohmann::json &j, vec3f &v)
{
  j.at(0).get_to(v.x);
  j.at(1).get_to(v.y);
  j.at(2).get_to(v.z);
}

inline void to_json(nlohmann::json &j, const LinearSpace3f &ls)
{
  j = nlohmann::json{{"x", ls.vx}, {"y", ls.vy}, {"z", ls.vz}};
}

inline void from_json(const nlohmann::json &j, LinearSpace3f &ls)
{
  j.at("x").get_to(ls.vx);
  j.at("y").get_to(ls.vy);
  j.at("z").get_to(ls.vz);
}

inline void to_json(nlohmann::json &j, const AffineSpace3f &as)
{
  j = nlohmann::json{{"linear", as.l}, {"affine", as.p}};
}

inline void from_json(const nlohmann::json &j, AffineSpace3f &as)
{
  j.at("linear").get_to(as.l);
  j.at("affine").get_to(as.p);
}

inline void to_json(nlohmann::json &j, const quaternionf &q)
{
  j = nlohmann::json{{"r", q.r}, {"i", q.i}, {"j", q.j}, {"k", q.k}};
}

inline void from_json(const nlohmann::json &j, quaternionf &q)
{
  j.at("r").get_to(q.r);
  j.at("i").get_to(q.i);
  j.at("j").get_to(q.j);
  j.at("k").get_to(q.k);
}

} // namespace math

namespace utility {

inline void to_json(nlohmann::json &j, const Any &a)
{
  if (a.is<int>())
    j = a.get<int>();
  else if (a.is<bool>())
    j = a.get<bool>();
  else if (a.is<float>())
    j = a.get<float>();
  else if (a.is<std::string>())
    j = a.get<std::string>();
  else if (a.is<math::vec2f>())
    j = a.get<math::vec2f>();
  else if (a.is<math::vec3f>())
    j = a.get<math::vec3f>();
  else if (a.is<math::AffineSpace3f>())
    j = a.get<math::AffineSpace3f>();
  else
    j = ":^)";
}

inline void from_json(const nlohmann::json &j, Any &a)
{
  // ALOK: does this really work???
  j.get_to(a);
}

} // namespace utility

} // namespace rkcommon

///////////////////////////////////////////////////////////////////////
// Global namespace type definitions //////////////////////////////////
///////////////////////////////////////////////////////////////////////

inline void to_json(nlohmann::json &j, const CameraState &cs)
{
  j = nlohmann::json{{"centerTranslation", cs.centerTranslation},
      {"translation", cs.translation},
      {"rotation", cs.rotation}};
}

inline void from_json(const nlohmann::json &j, CameraState &cs)
{
  j.at("centerTranslation").get_to(cs.centerTranslation);
  j.at("translation").get_to(cs.translation);
  j.at("rotation").get_to(cs.rotation);
}
