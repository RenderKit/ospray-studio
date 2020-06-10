// ======================================================================== //
// Copyright 2020 Intel Corporation                                         //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#pragma once

#include "Exporter.h"

namespace ospray::sg {

  struct OSPSG_INTERFACE ImageExporter : public Exporter
  {
    ImageExporter() = default;
    ~ImageExporter() = default;

    void setImageData(const void *fb, vec2i size, std::string format);

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

  inline void ImageExporter::floatToChar()
  {
    const float *fb = (const float *)child("data").valueAs<const void *>();
    vec2i size = child("size").valueAs<vec2i>();
    size_t nsubpix = 4 * size.x * size.y;
    char *newfb = (char *)malloc(nsubpix * sizeof(char));
    for (size_t i = 0; i < nsubpix; i++)
      newfb[i] = char(255 * fb[i]);
    child("data") = (const void *)newfb;
  }

  inline void ImageExporter::charToFloat()
  {
    const char *fb = (const char *)child("data").valueAs<const void *>();
    vec2i size = child("size").valueAs<vec2i>();
    size_t nsubpix = 4 * size.x * size.y;
    float *newfb = (float *)malloc(nsubpix * sizeof(float));
    for (size_t i = 0; i < nsubpix; i++)
      newfb[i] = fb[i] * 0.00392156862f;
    child("data") = (const void *)newfb;
  }

}  // namespace ospray::sg
