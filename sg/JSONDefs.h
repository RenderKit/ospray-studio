// Copyright 2009-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <json.hpp>

#include "Node.h"
#include "importer/Importer.h"
#include "scene/lights/LightsManager.h"

// This file contains definitions of `to_json` and `from_json` for custom types
// used within Studio. These methods allow us to easily serialize and
// deserialize SG nodes to JSON.

// default nlohmann::json will sort map alphabetically. this leaves it as-is
using JSON = nlohmann::ordered_json;

///////////////////////////////////////////////////////////////////////
// Forward declarations ///////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

inline void to_json(JSON &j, const CameraState &cs);
inline void from_json(const JSON &j, CameraState &cs);

// rkcommon type declarations /////////////////////////////////////////

namespace rkcommon {
namespace containers {
void to_json(
    JSON &j, const FlatMap<std::string, ospray::sg::NodePtr> &fm);
void from_json(
    const JSON &j, FlatMap<std::string, ospray::sg::NodePtr> &fm);
} // namespace containers
namespace math {
inline void to_json(JSON &j, const LinearSpace2f &as);
inline void from_json(const JSON &j, LinearSpace2f &as);
inline void to_json(JSON &j, const AffineSpace3f &as);
inline void from_json(const JSON &j, AffineSpace3f &as);
inline void to_json(JSON &j, const quaternionf &q);
inline void from_json(const JSON &j, quaternionf &q);
} // namespace math
namespace utility {
void to_json(JSON &j, const Any &a);
void from_json(const JSON &j, Any &a);
} // namespace utility
} // namespace rkcommon

///////////////////////////////////////////////////////////////////////
// SG types ///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

namespace ospray {
namespace sg {

inline void to_json(JSON &j, const Node &n)
{
  // Don't export these nodes, they must be regenerated and can't be imported.
  if ((n.type() == NodeType::GENERIC && n.name() == "handles")
      || (n.type() == NodeType::PARAMETER && n.subType() == "Data")
      || (n.type() == NodeType::GEOMETRY))
    return;

  // XXX would be nice to have GEOMETRY node for isVisible and isClippingGeometry properties

  j = JSON{{"name", n.name()},
      {"type", NodeTypeToString[n.type()]},
      {"subType", n.subType()}};

  // Don't export the node descriptions to JSON.  They take a lot of space, yet
  // provide little value. (20-25% of a .sg file for descriptions)
  //if (n.description() != "<no description>")
  //  j["description"] = n.description();

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

  if (n.type() == NodeType::PARAMETER)
    j["sgOnly"] = n.sgOnly();

  if (n.value().valid() && (n.type() == NodeType::PARAMETER
      || n.type() == NodeType::TRANSFORM)) {
    j["value"] = n.value();
    if (n.hasMinMax())
      j["minMax"] = JSON{n.min(), n.max()};
  }

  if (n.hasChildren())
    j["children"] = n.children();
}

inline void from_json(const JSON &, Node &) {}

inline OSPSG_INTERFACE NodePtr createNodeFromJSON(const JSON &j) {
  NodePtr n = nullptr;

  // This is a generated value and can't be imported
  if (j["name"] == "handles")
    return nullptr;

  // XXX Textures import needs to be handled correctly.  Skip for now.
  if (j["subType"] == "texture_2d")
    return nullptr;

  // If the json doesn't contain a valid value, just ignore the node
  // :^) was used as a sentinel for unhandled types.
  if ((j.contains("value") && j["value"].is_string()
          && j["value"].get<std::string>() == ":^)"))
    return nullptr;

  if (j.contains("value")) {
    Any value;

    // Stored in scene file as basic JSON objects.  Rather than trying to infer
    // their subType in generic from_json(), it's easier to handle type here.
    if (j["subType"] == "transform") {
      value = j["value"].get<rkcommon::math::AffineSpace3f>();
    } else if (j["subType"] == "quaternionf") {
      value = j["value"].get<rkcommon::math::quaternionf>();
    } else if (j["subType"] == "linear2f") {
      value = j["value"].get<rkcommon::math::LinearSpace2f>();
    } else {
      // Any other data type
      value = j["value"].get<Any>();
    }

    // Create node with optional description
    if (j.contains("description"))
      n = createNode(j["name"], j["subType"], j["description"], value);
    else
      n = createNode(j["name"], j["subType"], value);

    if (j.contains("sgOnly") && j["sgOnly"].get<bool>())
      n->setSGOnly();

    if (j.contains("minMax")) {
      auto minMax = j["minMax"].get<std::vector<Any>>();
      n->setMinMax(minMax[0], minMax[1]);
    }

    // JSON doesn't distinguish the following types.
    // Need conversion based on subType.

    // integer to uint8_t
    if (j["subType"] == "uchar") {
      n->setValue(uint8_t(n->valueAs<int>()));
      if (n->hasMinMax())
        n->setMinMax(uint8_t(n->minAs<int>()), uint8_t(n->maxAs<int>()));
    }

    // vec2f to range1f
    if (j["subType"] == "range1f") {
      vec2f v(n->valueAs<vec2f>());
      n->setValue(math::range1f(v[0], v[1]));
      if (n->hasMinMax())
        n->setMinMax(range1f(n->minAs<float>()), range1f(n->maxAs<float>()));
    }
  } else {
    n = createNode(j["name"], j["subType"]);
  }

  if (n != nullptr) {
    // the default ambient light might not exist in this scene
    // the loop below will add it if it does exist
    if (n && n->type() == NodeType::LIGHTS)
      n->nodeAs<LightsManager>()->removeLight("ambient");

    if (j.contains("children")) {
      for (auto &jChild : j["children"]) {
        auto child = createNodeFromJSON(jChild);
        if (!child)
          continue;
        if (n->type() == NodeType::LIGHTS)
          n->nodeAs<LightsManager>()->addLight(child);
        else if (n->type() == NodeType::MATERIAL)
          n->nodeAs<Material>()->add(child);
        else
          n->add(child);
      }
    }
  }

  return n;
}

} // namespace sg
} // namespace ospray

///////////////////////////////////////////////////////////////////////
// rkcommon type definitions //////////////////////////////////////////
///////////////////////////////////////////////////////////////////////

namespace rkcommon {
namespace containers {

inline void to_json(
    JSON &j, const FlatMap<std::string, ospray::sg::NodePtr> &fm)
{
  for (const auto e : fm) {
    JSON jnew = *(e.second);
    if (!jnew.is_null())
      j.push_back(jnew);
  }
}

inline void from_json(
    const JSON &, FlatMap<std::string, ospray::sg::NodePtr> &)
{}

} // namespace containers

namespace math {

inline void to_json(JSON &j, const vec2i &v)
{
  j = {v.x, v.y};
}

inline void from_json(const JSON &j, vec2i &v)
{
  j.at(0).get_to(v.x);
  j.at(1).get_to(v.y);
}

inline void to_json(JSON &j, const vec2f &v)
{
  j = {v.x, v.y};
}

inline void to_json(JSON &j, const range1f &v)
{
  j = {v.lower, v.upper};
}

inline void from_json(const JSON &j, vec2f &v)
{
  j.at(0).get_to(v.x);
  j.at(1).get_to(v.y);
}

inline void to_json(JSON &j, const vec3i &v)
{
  j = {v.x, v.y, v.z};
}

inline void from_json(const JSON &j, vec3i &v)
{
  j.at(0).get_to(v.x);
  j.at(1).get_to(v.y);
  j.at(2).get_to(v.z);
}

inline void to_json(JSON &j, const vec3f &v)
{
  j = {v.x, v.y, v.z};
}

inline void from_json(const JSON &j, vec3f &v)
{
  j.at(0).get_to(v.x);
  j.at(1).get_to(v.y);
  j.at(2).get_to(v.z);
}

inline void to_json(JSON &j, const vec4i &v)
{
  j = {v.x, v.y, v.z, v.w};
}

inline void from_json(const JSON &j, vec4i &v)
{
  j.at(0).get_to(v.x);
  j.at(1).get_to(v.y);
  j.at(2).get_to(v.z);
  j.at(2).get_to(v.w);
}

inline void to_json(JSON &j, const vec4f &v)
{
  j = {v.x, v.y, v.z, v.w};
}

inline void from_json(const JSON &j, vec4f &v)
{
  j.at(0).get_to(v.x);
  j.at(1).get_to(v.y);
  j.at(2).get_to(v.z);
  j.at(3).get_to(v.w);
}

inline void to_json(JSON &j, const LinearSpace2f &ls)
{
  j = JSON{{"x", ls.vx}, {"y", ls.vy}};
}

inline void from_json(const JSON &j, LinearSpace2f &ls)
{
  j.at("x").get_to(ls.vx);
  j.at("y").get_to(ls.vy);
}


inline void to_json(JSON &j, const LinearSpace3f &ls)
{
  j = JSON{{"x", ls.vx}, {"y", ls.vy}, {"z", ls.vz}};
}

inline void from_json(const JSON &j, LinearSpace3f &ls)
{
  j.at("x").get_to(ls.vx);
  j.at("y").get_to(ls.vy);
  j.at("z").get_to(ls.vz);
}

inline void to_json(JSON &j, const AffineSpace3f &as)
{
  j = JSON{{"linear", as.l}, {"affine", as.p}};
}

inline void from_json(const JSON &j, AffineSpace3f &as)
{
  if (j.contains("linear") && j.contains("affine")) {
    j.at("linear").get_to(as.l);
    j.at("affine").get_to(as.p);
  } else {
    std::vector<float> xfm;
    for (auto &val : j)
      xfm.push_back(val.get<float>());
    as = rkcommon::math::affine3f(&xfm[0], &xfm[4], &xfm[8], &xfm[12]);
  }
}

inline void to_json(JSON &j, const quaternionf &q)
{
  j = JSON{{"r", q.r}, {"i", q.i}, {"j", q.j}, {"k", q.k}};
}

inline void from_json(const JSON &j, quaternionf &q)
{
  j.at("r").get_to(q.r);
  j.at("i").get_to(q.i);
  j.at("j").get_to(q.j);
  j.at("k").get_to(q.k);
}

} // namespace math

namespace utility {

inline void to_json(JSON &j, const Any &a)
{
  if (a.is<int>())
    j = a.get<int>();
  else if (a.is<bool>())
    j = a.get<bool>();
  else if (a.is<uint8_t>())
    j = a.get<uint8_t>();
  else if (a.is<float>())
    j = a.get<float>();
  else if (a.is<std::string>())
    j = a.get<std::string>();
  else if (a.is<math::vec2i>())
    j = a.get<math::vec2i>();
  else if (a.is<math::vec2f>())
    j = a.get<math::vec2f>();
  else if (a.is<math::range1f>())
    j = a.get<math::range1f>();
  else if (a.is<math::vec3i>())
    j = a.get<math::vec3i>();
  else if (a.is<math::vec3f>())
    j = a.get<math::vec3f>();
  else if (a.is<math::vec4f>())
    j = a.get<math::vec4f>();
  else if (a.is<math::LinearSpace2f>())
    j = a.get<math::LinearSpace2f>();
  else if (a.is<math::AffineSpace3f>())
    j = a.get<math::AffineSpace3f>();
  else if (a.is<math::quaternionf>())
    j = a.get<math::quaternionf>();
  else {
    std::cerr << "JSONDefs.h: :^) strikes back!!!" << std::endl;
    j = ":^)";
  }
}

inline void from_json(const JSON &j, Any &a)
{
  if (j.is_primitive()) { // string, number , bool, null, or binary
    if (j.is_null())
      return;
    else if (j.is_boolean())
      a = j.get<bool>();
    else if (j.is_number_integer())
      a = j.get<int>();
    else if (j.is_number_float())
      a = j.get<float>();
    else if (j.is_string())
      a = j.get<std::string>();
    else
      std::cout << "unhandled primitive type in json" << std::endl;
  } else if (j.is_structured()) { // array or object
    if (j.is_array() && j.size() == 2) {
      if (j[0].is_number_float())
        a = j.get<math::vec2f>();
      else
        a = j.get<math::vec2i>();
    } else if (j.is_array() && j.size() == 3) {
      if (j[0].is_number_float())
        a = j.get<math::vec3f>();
      else
        a = j.get<math::vec3i>();
    } else if (j.is_array() && j.size() == 4) {
      if (j[0].is_number_float())
        a = j.get<math::vec4f>();
      else
        a = j.get<math::vec4i>();
    } else if (j.is_object()) {
      std::cout << "cannot load object types from json" << std::endl;
      for (auto &jI : j.items()) {
        auto &key = jI.key();
        auto &val = jI.value();
        std::cout << ": " << key << ":" << val << std::endl;
      }
    } else
      std::cout << "unhandled structured type in json " << std::endl;
  } else { // something is wrong
    std::cout << "unidentified type in json" << std::endl;
  }
}

} // namespace utility

} // namespace rkcommon

///////////////////////////////////////////////////////////////////////
// Global namespace type definitions //////////////////////////////////
///////////////////////////////////////////////////////////////////////

inline void to_json(JSON &j, const CameraState &cs)
{
  j = JSON{{"centerTranslation", cs.centerTranslation},
      {"translation", cs.translation},
      {"rotation", cs.rotation}};
}

inline void from_json(const JSON &j, CameraState &cs)
{
  j.at("centerTranslation").get_to(cs.centerTranslation);
  j.at("translation").get_to(cs.translation);
  j.at("rotation").get_to(cs.rotation);
}
