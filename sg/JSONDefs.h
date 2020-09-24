// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <json.hpp>

#include "Node.h"
#include "importer/Importer.h"

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

  if (n.value().valid() && n.type() == NodeType::PARAMETER)
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
  else
    j = ":^)";
}

inline void from_json(const nlohmann::json &j, Any &a) {}

} // namespace utility

} // namespace rkcommon
