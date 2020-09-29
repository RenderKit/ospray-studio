// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "Exporter.h"

#define ONEOVER255 0.00392156862f

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

  inline void ImageExporter::setImageData(const void *fb,
                                          vec2i size,
                                          std::string format)
  {
    createChild("data", "void_ptr", fb);
    createChild("size", "vec2i", size);
    createChild("format", "string", format);
  }

  inline void ImageExporter::setAdditionalLayer(std::string layerName,
                                                const void *fb)
  {
    createChild(layerName, "void_ptr", fb);
  }

  inline void ImageExporter::clearLayer(std::string layerName)
  {
    remove(layerName);
  }

  inline void ImageExporter::floatToChar()
  {
    const float *fb = (const float *)child("data").valueAs<const void *>();
    vec2i size = child("size").valueAs<vec2i>();
    size_t nsubpix = 4 * size.x * size.y;
    char *newfb = (char *)malloc(nsubpix * sizeof(char));
    if (newfb) {
      // cubic gamma approximation to avoid pow(),
      // http://stereopsis.com/polygamma.html
      // ax^3 + bx^2 + cx = 0, where a = 1.49, b = -3.23, and c = 2.74
      auto fastGamma = [](float x) -> float {
        return 1.49 * x * x * x - 3.23 * x * x + 2.74 * x;
      };

      for (size_t i = 0; i < nsubpix; i++) {
        float val = std::max(std::min(fb[i], 1.f), 0.f);
        newfb[i] = char(255 * fastGamma(val));
      }

      child("data") = (const void *)newfb;
    }

    if (hasChild("depth")) {
      char *newdb = (char *)malloc(nsubpix * sizeof(char));
      if (newdb)
        for (size_t i = 0; i < nsubpix; i++)
          newdb[i] = char(255 * fb[i]);
      child("depth") = (const void *)newdb;
    }
  }

  inline void ImageExporter::charToFloat()
  {
    const char *fb = (const char *)child("data").valueAs<const void *>();
    vec2i size = child("size").valueAs<vec2i>();
    size_t nsubpix = 4 * size.x * size.y;
    float *newfb = (float *)malloc(nsubpix * sizeof(float));
    if (newfb)
      for (size_t i = 0; i < nsubpix; i++)
        newfb[i] = fb[i] * ONEOVER255;
    child("data") = (const void *)newfb;
  }

  }  // namespace sg
} // namespace ospray
