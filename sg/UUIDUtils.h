// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "sg/Node.h"

#include <climits>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <unordered_map>

// typedef std::vector<std::uint32_t> UUID;
typedef struct _UUID{
  uint32_t data0;
  uint32_t data1;
  uint32_t data2;
  uint32_t data3;
} UUID;

inline UUID makeUUID(uint32_t val0, uint32_t val1, uint32_t val2, uint32_t val3) {
  UUID uuid;
  uuid.data0 = val0;
  uuid.data1 = val1;
  uuid.data2 = val2;
  uuid.data3 = val3;
  return uuid;
}

// convert hex string to a UUID format(implemented as a vector, defined above)
inline UUID makeUUID(std::string &uuidString)
{
  // UUID format for storage
  // 4 32-bit sized integers representation
  UUID uuid;

  // remove occurences of '-' hyphens, which only exist for reading purposes
  std::string uuidCompact{uuidString};
  uuidCompact.erase(
      remove(uuidCompact.begin(), uuidCompact.end(), '-'), uuidCompact.end());

  size_t pos = 8;
  std::vector<std::string> tokens;

  while (!uuidCompact.empty()) {
    std::string token;
    token = uuidCompact.substr(0, pos);
    tokens.push_back(token);
    uuidCompact.erase(0, pos);
  }

  uuid.data0 = (uint32_t) std::strtol(tokens[0].c_str(), NULL, 16);
  uuid.data1 = (uint32_t) std::strtol(tokens[1].c_str(), NULL, 16);
  uuid.data2 = (uint32_t) std::strtol(tokens[2].c_str(), NULL, 16);
  uuid.data3 = (uint32_t)std::strtol(tokens[3].c_str(), NULL, 16);

  return uuid;
}

// TODO:: implement instanceID and world coords

struct FrameMetadata
{
  UUID modelId;
};

typedef std::unordered_map<OSPGeometricModel, std::string> UUIDMap;
