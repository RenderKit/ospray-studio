// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "Exporter.h"

#define ONEOVER255 1.f / 255.f

namespace ospray {
namespace sg {

struct OSPSG_INTERFACE ImageExporter : public Exporter
{
  ImageExporter() = default;
  ~ImageExporter() = default;

  void setImageData(const void *fb, vec2i size, std::string format);
  void setAdditionalLayer(std::string layerName, const void *fb);
  void clearLayer(std::string layerName);

 protected:
  void floatToChar();
  void charToFloat();
};

// ImageExporter functions //////////////////////////////////////////////////

inline void ImageExporter::setImageData(
    const void *fb, vec2i size, std::string format)
{
  createChild("data", "void_ptr", fb);
  createChild("size", "vec2i", size);
  createChild("format", "string", format);
}

inline void ImageExporter::setAdditionalLayer(
    std::string layerName, const void *fb)
{
  createChild(layerName, "void_ptr", fb);
}

inline void ImageExporter::clearLayer(std::string layerName)
{
  remove(layerName);
}

inline void ImageExporter::floatToChar()
{
  const vec4f *fb = (const vec4f *)child("data").valueAs<const void *>();
  vec2i size = child("size").valueAs<vec2i>();
  size_t npix = size.x * size.y;
  vec4uc *newfb = (vec4uc *)malloc(npix * sizeof(vec4uc));
  if (newfb) {
    for (size_t i = 0; i < npix; i++) {
      auto gamma = [](float x) -> float {
        return pow(std::max(std::min(x, 1.f), 0.f), 1.f / 2.2f);
      };
      newfb[i].x = uint8_t(255 * gamma(fb[i].x));
      newfb[i].y = uint8_t(255 * gamma(fb[i].y));
      newfb[i].z = uint8_t(255 * gamma(fb[i].z));
      newfb[i].w = uint8_t(255 * std::max(std::min(fb[i].w, 1.f), 0.f));
    }

    child("data") = (const void *)newfb;
  }

  if (hasChild("depth")) {
    const float *db = (const float *)child("depth").valueAs<const void *>();
    uint8_t *newdb = (uint8_t *)malloc(npix * sizeof(uint8_t));
    if (newdb) {
      for (size_t i = 0; i < npix; i++)
        newdb[i] = uint8_t(255 * db[i]);
      child("depth") = (const void *)newdb;
    }
  }
}

inline void ImageExporter::charToFloat()
{
  const uint8_t *fb = (const uint8_t *)child("data").valueAs<const void *>();
  vec2i size = child("size").valueAs<vec2i>();
  size_t nsubpix = 4 * size.x * size.y;
  float *newfb = (float *)malloc(nsubpix * sizeof(float));
  if (newfb) {
    for (size_t i = 0; i < nsubpix; i++)
      newfb[i] = fb[i] * ONEOVER255;
    child("data") = (const void *)newfb;
  }
}

} // namespace sg
} // namespace ospray
